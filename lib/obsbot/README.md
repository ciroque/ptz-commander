# OBSBOT SDK

`libdev.dll` and `libdev.lib` are the OBSBOT device SDK. The Windows build links the import library, copies the DLL next to `ptz_commander.exe`, and the MSI installs that DLL. The matching headers are `include/obsbot/`.

The SDK drop in this tree has no license file, and the headers do not carry one. Nothing checked into the repo grants the right to redistribute these binaries. They stay because the app does not build or run the OBSBOT adapter without them, and there is no source replacement in this project.

`libdev.dll` imports Windows system libraries only. It does not import `w32-pthreads.dll`. The previous SDK copy, its headers, the PDBs, and the unused pthread binaries were removed.

Do not commit `*.pdb` files next to this DLL.
