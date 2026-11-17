#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace nd::io {
struct Chapter {
    std::string title;
    std::size_t byte_start{};
    std::size_t byte_length{};
};
struct ScriptDocument {
    std::string text;
    std::vector<Chapter> chapters;
    std::filesystem::path source;
    bool converted_encoding{};
};
// Supports UTF-8 (with/without BOM), UTF-16 LE/BE, plaintext/Markdown and basic HTML.
// Complex formats report an actionable unsupported-format error instead of silent corruption.
ScriptDocument open_script(const std::filesystem::path& path);
ScriptDocument decode_script(std::string bytes, std::string extension = {});
std::string utf8_from_utf16(const std::string& bytes, bool little_endian);
}  // namespace nd::io
