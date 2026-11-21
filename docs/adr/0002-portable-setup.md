# ADR 0002: Portable setup files

- **Status:** Accepted
- **Date:** 2026-11-20
- **Deciders:** project maintainer
- **Related:** [ADR 0001](0001-scenes.md); ONVIF passwords in `docs/ARCHITECTURE.md`

## Context

A `.ptzc` file should move to another PC or Windows account with the same cameras. ONVIF passwords are stored as Windows DPAPI blobs, which only the user who saved them can decrypt. That is the right binding for a file that lives in `%LOCALAPPDATA%`, and it matches a Remote Desktop `.rdp` file: the password is bound to the user, and anywhere else the client asks again.

Leaving out a camera whose password cannot be decrypted drops its presets on the next `setup save`. Scene bindings name that camera's serial, and the preset they name is gone with the camera entry.

## Decision

1. A setup file is portable. `setup export` writes the same JSON as `setup save` with no `password` key on `onvif` entries.
2. `setup save` writes a `dpapi:` blob for a password the process has. An inactive camera with no password omits that key. A `dpapi:` blob this user cannot decrypt is written back unchanged until a new password replaces it.
3. `setup load` asks when an `onvif` entry has no password, or its `dpapi:` blob does not decrypt for this user. The prompt is hidden. A blank answer skips. Input that is not a console skips without asking.
4. The password is tried immediately. An authentication fault asks again, up to three times. A camera that does not answer is still left out.
5. A skipped camera stays in the setup, inactive. It is listed, it cannot be moved, and it keeps its presets and scene bindings. `camera login` sets the password, connects, and makes it active.
6. The loader takes a password callback. The REPL passes the hidden-input reader. Tests pass a stub. `setup load` is REPL-only. The WebSocket accepts scene apply and nothing else, because a load can block on input.
7. Older plaintext passwords, and `dpapi:` blobs that decrypt, load without a prompt.

## Consequences

- Copying an exported file does not reveal ONVIF passwords.
- Each Windows user is asked once. The next `setup save` binds the entered password to that user.
- A skipped camera survives a later save. Its presets and scene bindings are still there for `camera login`.
- Skipping a camera whose blob does not decrypt keeps that blob in the next save, so the Windows user who saved it can still open it.
- A saved password for a camera that is switched off still does not keep that camera. That is the same as today.
