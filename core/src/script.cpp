#include "nandadhvani/io/script.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
namespace nd::io {
namespace {
void append_cp(std::string& o, std::uint32_t c) {
    if (c <= 0x7f)
        o.push_back(static_cast<char>(c));
    else if (c <= 0x7ff) {
        o.push_back(static_cast<char>(0xc0 | (c >> 6)));
        o.push_back(static_cast<char>(0x80 | (c & 63)));
    } else if (c <= 0xffff) {
        o.push_back(static_cast<char>(0xe0 | (c >> 12)));
        o.push_back(static_cast<char>(0x80 | ((c >> 6) & 63)));
        o.push_back(static_cast<char>(0x80 | (c & 63)));
    } else {
        o.push_back(static_cast<char>(0xf0 | (c >> 18)));
        o.push_back(static_cast<char>(0x80 | ((c >> 12) & 63)));
        o.push_back(static_cast<char>(0x80 | ((c >> 6) & 63)));
        o.push_back(static_cast<char>(0x80 | (c & 63)));
    }
}
std::string clean_html(const std::string& s) {
    std::string out;
    bool tag = false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '<') {
            tag = true;
            continue;
        }
        if (s[i] == '>') {
            tag = false;
            continue;
        }
        if (!tag) {
            if (s.compare(i, 4, "<br>") == 0) {
                out += '\n';
                i += 3;
            } else
                out += s[i];
        }
    }
    return out;
}
}  // namespace
std::string utf8_from_utf16(const std::string& b, bool le) {
    if (b.size() % 2)
        throw std::runtime_error("odd-length UTF-16 input");
    std::string o;
    for (std::size_t i = 0; i < b.size(); i += 2) {
        auto val = [&](std::size_t n) { return static_cast<unsigned char>(b[n]); };
        std::uint32_t cp = le ? (val(i) | (val(i + 1) << 8)) : (val(i + 1) | (val(i) << 8));
        if (cp >= 0xd800 && cp <= 0xdbff) {
            if (i + 3 >= b.size())
                throw std::runtime_error("truncated UTF-16 surrogate");
            std::uint32_t lo = le ? (val(i + 2) | (val(i + 3) << 8)) : (val(i + 3) | (val(i + 2) << 8));
            if (lo < 0xdc00 || lo > 0xdfff)
                throw std::runtime_error("invalid UTF-16 surrogate");
            cp = 0x10000 + ((cp - 0xd800) << 10) + (lo - 0xdc00);
            i += 2;
        } else if (cp >= 0xdc00 && cp <= 0xdfff)
            throw std::runtime_error("unpaired UTF-16 surrogate");
        append_cp(o, cp);
    }
    return o;
}
ScriptDocument decode_script(std::string b, std::string ext) {
    ScriptDocument d;
    ext = ext.empty() ? ".txt" : ext;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (b.size() >= 2 && static_cast<unsigned char>(b[0]) == 0xff &&
        static_cast<unsigned char>(b[1]) == 0xfe) {
        d.text = utf8_from_utf16(b.substr(2), true);
        d.converted_encoding = true;
    } else if (b.size() >= 2 && static_cast<unsigned char>(b[0]) == 0xfe &&
               static_cast<unsigned char>(b[1]) == 0xff) {
        d.text = utf8_from_utf16(b.substr(2), false);
        d.converted_encoding = true;
    } else {
        if (b.size() >= 3 && static_cast<unsigned char>(b[0]) == 0xef &&
            static_cast<unsigned char>(b[1]) == 0xbb && static_cast<unsigned char>(b[2]) == 0xbf)
            b.erase(0, 3);
        d.text = std::move(b);
    }
    if (ext == ".html" || ext == ".htm")
        d.text = clean_html(d.text);
    else if (ext == ".docx" || ext == ".odt" || ext == ".pdf" || ext == ".epub" || ext == ".rtf")
        throw std::runtime_error(
            "This format needs its optional importer; save as UTF-8 .txt or .html for now.");
    else if (ext != ".txt" && ext != ".md" && ext != ".markdown")
        throw std::runtime_error("Unsupported script file type: " + ext);
    if (ext == ".md" || ext == ".markdown") {
        std::size_t pos = 0;
        while (pos < d.text.size()) {
            auto end = d.text.find('\n', pos);
            if (end == std::string::npos)
                end = d.text.size();
            std::size_t p = pos;
            while (p < end && d.text[p] == '#')
                ++p;
            if (p > pos && p - pos <= 6 && p < end && d.text[p] == ' ')
                d.chapters.push_back({d.text.substr(p + 1, end - p - 1), pos, end - pos});
            pos = end + 1;
        }
    }
    return d;
}
ScriptDocument open_script(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    if (!in)
        throw std::runtime_error("Cannot open script: " + p.string());
    std::string b((std::istreambuf_iterator<char>(in)), {});
    auto d = decode_script(std::move(b), p.extension().string());
    d.source = p;
    return d;
}
}  // namespace nd::io
