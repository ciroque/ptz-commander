#ifndef CAMERAS_ONVIF_ONVIF_PARSE_H
#define CAMERAS_ONVIF_ONVIF_PARSE_H

#include <cctype>
#include <cstdio>
#include <optional>
#include <string>

namespace cameras::onvif {

inline std::string trimCopy(std::string text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.erase(text.begin());
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back()))) {
        text.pop_back();
    }
    return text;
}

inline std::string xmlEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (const unsigned char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default: out.push_back(static_cast<char>(c)); break;
        }
    }
    return out;
}

inline std::string formatFixed(double value) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.6f", value);
    for (char* p = buf; *p != '\0'; ++p) {
        if (*p == ',') {
            *p = '.';
        }
    }
    return buf;
}

// Local-name tag boundary. "Media" does not match "GetMedia" or "MediaProfile".
inline size_t findXmlTag(const std::string& xml, const std::string& local, size_t from = 0) {
    size_t pos = xml.find(local, from);
    while (pos != std::string::npos) {
        const bool boundary = pos > 0 && (xml[pos - 1] == '<' || xml[pos - 1] == ':');
        const size_t after = pos + local.size();
        if (boundary && after < xml.size()) {
            const char c = xml[after];
            if (c == '>' || c == ' ' || c == '/' || c == '\t' || c == '\r' || c == '\n') {
                return pos;
            }
        }
        pos = xml.find(local, pos + local.size());
    }
    return std::string::npos;
}

inline std::string xmlElementText(const std::string& xml, const std::string& local, size_t from = 0) {
    const size_t pos = findXmlTag(xml, local, from);
    if (pos == std::string::npos) {
        return {};
    }
    const size_t gt = xml.find('>', pos);
    if (gt == std::string::npos || (gt > 0 && xml[gt - 1] == '/')) {
        return {};
    }
    const size_t lt = xml.find('<', gt + 1);
    if (lt == std::string::npos) {
        return {};
    }
    return trimCopy(xml.substr(gt + 1, lt - gt - 1));
}

inline std::string xmlStartTag(const std::string& xml, const std::string& local, size_t from = 0) {
    const size_t pos = findXmlTag(xml, local, from);
    if (pos == std::string::npos) {
        return {};
    }
    const size_t lt = xml.rfind('<', pos);
    const size_t gt = xml.find('>', pos);
    if (lt == std::string::npos || gt == std::string::npos) {
        return {};
    }
    return xml.substr(lt, gt - lt + 1);
}

inline std::string xmlAttr(const std::string& tag, const std::string& name) {
    const std::string key = name + "=\"";
    size_t pos = 0;
    while ((pos = tag.find(key, pos)) != std::string::npos) {
        if (pos == 0 || std::isspace(static_cast<unsigned char>(tag[pos - 1]))) {
            const size_t start = pos + key.size();
            const size_t end = tag.find('"', start);
            if (end == std::string::npos) {
                return {};
            }
            return tag.substr(start, end - start);
        }
        pos += key.size();
    }
    return {};
}

struct HttpUrl {
    bool https = false;
    std::string host;
    int port = 80;
    std::string path = "/";
};

inline std::optional<HttpUrl> parseHttpUrl(const std::string& url) {
    const std::string http = "http://";
    const std::string https = "https://";
    HttpUrl out;
    std::string rest;
    if (url.compare(0, http.size(), http) == 0) {
        rest = url.substr(http.size());
        out.port = 80;
    } else if (url.compare(0, https.size(), https) == 0) {
        rest = url.substr(https.size());
        out.https = true;
        out.port = 443;
    } else {
        return std::nullopt;
    }

    const size_t slash = rest.find('/');
    const std::string hostPort = slash == std::string::npos ? rest : rest.substr(0, slash);
    out.path = slash == std::string::npos ? "/" : rest.substr(slash);
    const size_t query = out.path.find('?');
    if (query != std::string::npos) {
        out.path.resize(query);
    }

    const size_t colon = hostPort.rfind(':');
    if (colon != std::string::npos && hostPort.find(':') == colon) {
        out.host = hostPort.substr(0, colon);
        int port = 0;
        for (size_t i = colon + 1; i < hostPort.size(); ++i) {
            const char c = hostPort[i];
            if (c < '0' || c > '9') {
                return std::nullopt;
            }
            port = port * 10 + (c - '0');
            if (port > 65535) {
                return std::nullopt;
            }
        }
        if (port <= 0) {
            return std::nullopt;
        }
        out.port = port;
    } else {
        out.host = hostPort;
    }
    if (out.host.empty() || out.path.empty()) {
        return std::nullopt;
    }
    return out;
}

inline std::string urlDecode(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size()
            && std::isxdigit(static_cast<unsigned char>(text[i + 1]))
            && std::isxdigit(static_cast<unsigned char>(text[i + 2]))) {
            auto hex = [](char c) {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return 0;
            };
            out.push_back(static_cast<char>((hex(text[i + 1]) << 4) | hex(text[i + 2])));
            i += 2;
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

} // namespace cameras::onvif

#endif
