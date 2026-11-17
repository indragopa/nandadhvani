#include "nandadhvani/core/project.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
namespace nd {
namespace {
std::string path_utf8(const std::filesystem::path& p) {
    const auto s = p.u8string();
    return std::string(reinterpret_cast<const char*>(s.data()), s.size());
}
std::filesystem::path path_from_utf8(const std::string& s) {
    return std::filesystem::u8path(s);
}
std::string esc(const std::string& s) {
    std::string o = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"':
                o += "\\\"";
                break;
            case '\\':
                o += "\\\\";
                break;
            case '\n':
                o += "\\n";
                break;
            case '\r':
                o += "\\r";
                break;
            case '\t':
                o += "\\t";
                break;
            default:
                if (c < 32) {
                    const char* h = "0123456789abcdef";
                    o += "\\u00";
                    o += h[c >> 4];
                    o += h[c & 15];
                } else
                    o += static_cast<char>(c);
        }
    }
    return o + '"';
}
class Parser {
    const std::string& s;
    std::size_t i = 0;

   public:
    explicit Parser(const std::string& x) : s(x) {}
    void ws() {
        while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i])))
            ++i;
    }
    bool eat(char c) {
        ws();
        if (i < s.size() && s[i] == c) {
            ++i;
            return true;
        }
        return false;
    }
    std::string str() {
        ws();
        if (i >= s.size() || s[i++] != '"')
            throw std::runtime_error("expected JSON string");
        std::string o;
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"')
                return o;
            if (c == '\\') {
                if (i >= s.size())
                    break;
                char e = s[i++];
                switch (e) {
                    case 'n':
                        o += '\n';
                        break;
                    case 'r':
                        o += '\r';
                        break;
                    case 't':
                        o += '\t';
                        break;
                    case '"':
                    case '\\':
                    case '/':
                        o += e;
                        break;
                    case 'u': {
                        if (i + 4 > s.size())
                            throw std::runtime_error("bad unicode escape");
                        unsigned cp = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = s[i++];
                            cp = cp * 16 + (h >= '0' && h <= '9'   ? h - '0'
                                            : h >= 'a' && h <= 'f' ? h - 'a' + 10
                                                                   : h - 'A' + 10);
                        }
                        if (cp < 128)
                            o += static_cast<char>(cp);
                        else if (cp < 2048) {
                            o += static_cast<char>(0xc0 | (cp >> 6));
                            o += static_cast<char>(0x80 | (cp & 63));
                        } else {
                            o += static_cast<char>(0xe0 | (cp >> 12));
                            o += static_cast<char>(0x80 | ((cp >> 6) & 63));
                            o += static_cast<char>(0x80 | (cp & 63));
                        }
                        break;
                    }
                    default:
                        throw std::runtime_error("bad JSON escape");
                }
            } else
                o += c;
        }
        throw std::runtime_error("unterminated JSON string");
    }
    std::string value() {
        ws();
        if (i < s.size() && s[i] == '"')
            return str();
        auto b = i;
        while (i < s.size() && s[i] != ',' && s[i] != ']' && s[i] != '}' &&
               !std::isspace(static_cast<unsigned char>(s[i])))
            ++i;
        return s.substr(b, i - b);
    }
    void skip() {
        ws();
        if (eat('{')) {
            while (!eat('}')) {
                (void)str();
                if (!eat(':'))
                    throw std::runtime_error("bad JSON");
                skip();
                if (eat('}'))
                    break;
                if (!eat(','))
                    throw std::runtime_error("bad JSON");
            }
        } else if (eat('[')) {
            while (!eat(']')) {
                skip();
                if (eat(']'))
                    break;
                if (!eat(','))
                    throw std::runtime_error("bad JSON");
            }
        } else
            (void)value();
    }
};
}  // namespace
void save_project(const std::filesystem::path& p, const ProjectData& d) {
    auto tmp = p;
    tmp += ".tmp";
    std::ofstream o(tmp, std::ios::binary | std::ios::trunc);
    if (!o)
        throw std::runtime_error("cannot create project");
    o << "{\n  \"format\": 1,\n  \"title\": " << esc(d.title)
      << ",\n  \"script_path\": " << esc(d.script_path) << ",\n  \"script_position\": " << d.script_position
      << ",\n  \"sample_rate\": " << d.sample_rate << ",\n  \"channels\": " << d.channels
      << ",\n  \"tracks\": [";
    for (std::size_t i = 0; i < d.tracks.size(); ++i) {
        auto& t = d.tracks[i];
        if (i)
            o << ',';
        o << "\n    {\"name\":" << esc(t.name) << ",\"type\":" << esc(t.type) << ",\"color\":" << esc(t.color)
          << ",\"gain\":" << t.gain << ",\"pan\":" << t.pan << ",\"muted\":" << (t.muted ? "true" : "false")
          << ",\"solo\":" << (t.solo ? "true" : "false") << ",\"locked\":" << (t.locked ? "true" : "false")
          << ",\"armed\":" << (t.armed ? "true" : "false") << ",\"media\":" << esc(path_utf8(t.media)) << '}';
    }
    o << "\n  ],\n  \"markers\": [";
    for (std::size_t i = 0; i < d.markers.size(); ++i) {
        if (i)
            o << ',';
        o << "{\"frame\":" << d.markers[i].frame << ",\"label\":" << esc(d.markers[i].label) << '}';
    }
    o << "]\n}\n";
    o.close();
    if (!o)
        throw std::runtime_error("failed writing project");
    std::error_code ec;
    std::filesystem::rename(tmp, p, ec);
    if (ec) {
        std::filesystem::remove(p, ec);
        ec.clear();
        std::filesystem::rename(tmp, p, ec);
        if (ec)
            throw std::runtime_error("cannot finalize project: " + ec.message());
    }
}
ProjectData load_project(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in)
        throw std::runtime_error("cannot open project");
    std::string s((std::istreambuf_iterator<char>(in)), {});
    Parser q(s);
    ProjectData d;
    if (!q.eat('{'))
        throw std::runtime_error("invalid project JSON");
    while (!q.eat('}')) {
        auto key = q.str();
        if (!q.eat(':'))
            throw std::runtime_error("invalid project JSON");
        if (key == "title")
            d.title = q.str();
        else if (key == "script_path")
            d.script_path = q.str();
        else if (key == "script_position")
            d.script_position = std::stoull(q.value());
        else if (key == "sample_rate")
            d.sample_rate = static_cast<std::uint32_t>(std::stoul(q.value()));
        else if (key == "channels")
            d.channels = static_cast<std::uint16_t>(std::stoul(q.value()));
        else if (key == "tracks") {
            if (!q.eat('['))
                throw std::runtime_error("invalid tracks");
            while (!q.eat(']')) {
                if (!q.eat('{'))
                    throw std::runtime_error("invalid track");
                TrackInfo t;
                while (!q.eat('}')) {
                    auto k = q.str();
                    if (!q.eat(':'))
                        throw std::runtime_error("invalid track");
                    auto v = q.value();
                    if (k == "name")
                        t.name = v;
                    else if (k == "type")
                        t.type = v;
                    else if (k == "color")
                        t.color = v;
                    else if (k == "gain")
                        t.gain = std::stof(v);
                    else if (k == "pan")
                        t.pan = std::stof(v);
                    else if (k == "muted")
                        t.muted = v == "true";
                    else if (k == "solo")
                        t.solo = v == "true";
                    else if (k == "locked")
                        t.locked = v == "true";
                    else if (k == "armed")
                        t.armed = v == "true";
                    else if (k == "media")
                        t.media = path_from_utf8(v);
                    else
                        throw std::runtime_error("unknown track property");
                    if (q.eat('}'))
                        break;
                    if (!q.eat(','))
                        throw std::runtime_error("invalid track delimiter");
                }
                d.tracks.push_back(std::move(t));
                if (q.eat(']'))
                    break;
                if (!q.eat(','))
                    throw std::runtime_error("invalid tracks delimiter");
            }
        } else if (key == "markers") {
            if (!q.eat('['))
                throw std::runtime_error("invalid markers");
            while (!q.eat(']')) {
                if (!q.eat('{'))
                    throw std::runtime_error("invalid marker");
                Marker m;
                while (!q.eat('}')) {
                    auto k = q.str();
                    if (!q.eat(':'))
                        throw std::runtime_error("invalid marker");
                    auto v = q.value();
                    if (k == "frame")
                        m.frame = std::stoull(v);
                    else if (k == "label")
                        m.label = v;
                    else
                        throw std::runtime_error("unknown marker property");
                    if (q.eat('}'))
                        break;
                    if (!q.eat(','))
                        throw std::runtime_error("invalid marker delimiter");
                }
                d.markers.push_back(std::move(m));
                if (q.eat(']'))
                    break;
                if (!q.eat(','))
                    throw std::runtime_error("invalid markers delimiter");
            }
        } else
            q.skip();
        if (q.eat('}'))
            break;
        if (!q.eat(','))
            throw std::runtime_error("invalid project delimiter");
    }
    return d;
}
std::filesystem::path autosave_path(const std::filesystem::path& p) {
    auto x = p;
    x += ".autosave";
    return x;
}
}  // namespace nd
