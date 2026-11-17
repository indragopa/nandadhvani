#include "nandadhvani/core/alignment.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
namespace nd {
namespace {
std::string norm(std::string s) {
    std::string o;
    for (unsigned char c : s) {
        if (c < 128) {
            if (std::isalnum(c))
                o.push_back(static_cast<char>(std::tolower(c)));
        } else
            o.push_back(static_cast<char>(c));
    }
    return o;
}
float similarity(const std::string& a, const std::string& b) {
    auto x = norm(a), y = norm(b);
    if (x == y)
        return 1.0F;
    if (x.empty() || y.empty())
        return 0;
    const auto n = x.size(), m = y.size();
    if (n > 128 || m > 128)
        return 0;
    std::size_t d[129][129]{};
    for (std::size_t i = 0; i <= n; ++i)
        d[i][0] = i;
    for (std::size_t j = 0; j <= m; ++j)
        d[0][j] = j;
    for (std::size_t i = 1; i <= n; ++i)
        for (std::size_t j = 1; j <= m; ++j)
            d[i][j] = std::min(
                {d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (x[i - 1] == y[j - 1] ? 0 : 1)});
    return 1.0F - static_cast<float>(d[n][m]) / static_cast<float>(std::max(n, m));
}
enum Op : unsigned char { Diag, Up, Left };
}  // namespace
std::vector<ScriptWord> tokenize_script(const std::string& s) {
    std::vector<ScriptWord> words;
    std::size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && static_cast<unsigned char>(s[i]) < 128 &&
               !std::isalnum(static_cast<unsigned char>(s[i])))
            ++i;
        if (i == s.size())
            break;
        std::size_t begin = i;
        if (static_cast<unsigned char>(s[i]) < 128) {
            while (i < s.size() && static_cast<unsigned char>(s[i]) < 128 &&
                   std::isalnum(static_cast<unsigned char>(s[i])))
                ++i;
        } else {
            while (i < s.size() && static_cast<unsigned char>(s[i]) >= 128)
                ++i;  // Keep each contiguous complex-script token as a word.
        }
        words.push_back({s.substr(begin, i - begin), begin, i - begin});
    }
    return words;
}
std::vector<WordMapping> align_script(const std::vector<ScriptWord>& s, const std::vector<TimedWord>& a,
                                      double threshold) {
    if (threshold < 0 || threshold > 1)
        throw std::invalid_argument("threshold outside 0..1");
    if (s.empty())
        return {};
    if (s.size() > 200000 || a.size() > 200000 || s.size() * (a.size() + 1) > 30000000)
        throw std::length_error("alignment too large; process in chunks");
    const std::size_t cols = a.size() + 1;
    std::vector<float> score((s.size() + 1) * cols);
    std::vector<Op> trace(score.size());
    auto ix = [&](std::size_t i, std::size_t j) { return i * cols + j; };
    constexpr float gap = -0.72F;
    for (std::size_t i = 1; i <= s.size(); ++i) {
        score[ix(i, 0)] = static_cast<float>(i) * gap;
        trace[ix(i, 0)] = Up;
    }
    for (std::size_t j = 1; j <= a.size(); ++j) {
        score[ix(0, j)] = static_cast<float>(j) * gap;
        trace[ix(0, j)] = Left;
    }
    for (std::size_t i = 1; i <= s.size(); ++i)
        for (std::size_t j = 1; j <= a.size(); ++j) {
            float sim = similarity(s[i - 1].text, a[j - 1].text);
            float d =
                score[ix(i - 1, j - 1)] + (sim >= threshold ? (0.65F + 0.35F * sim) : (-0.9F + 0.3F * sim));
            float u = score[ix(i - 1, j)] + gap, v = score[ix(i, j - 1)] + gap;
            auto k = ix(i, j);
            if (d >= u && d >= v) {
                score[k] = d;
                trace[k] = Diag;
            } else if (u >= v) {
                score[k] = u;
                trace[k] = Up;
            } else {
                score[k] = v;
                trace[k] = Left;
            }
        }
    std::vector<WordMapping> out;
    std::size_t i = s.size(), j = a.size();
    while (i || j) {
        auto op = trace[ix(i, j)];
        if (i && j && op == Diag) {
            float sim = similarity(s[i - 1].text, a[j - 1].text);
            out.push_back({i - 1, j - 1,
                           sim >= threshold ? AlignmentStatus::Matched : AlignmentStatus::Different,
                           a[j - 1].start_seconds, a[j - 1].end_seconds, sim});
            --i;
            --j;
        } else if (i && (j == 0 || op == Up)) {
            out.push_back({i - 1, 0, AlignmentStatus::Missing, 0, 0, 0});
            --i;
        } else if (j) {
            if (!out.empty() && out.back().status == AlignmentStatus::Matched &&
                norm(s[out.back().script_index].text) == norm(a[j - 1].text)) {
                auto m = out.back();
                m.status = AlignmentStatus::Repeated;
                m.start_seconds = a[j - 1].start_seconds;
                m.end_seconds = a[j - 1].end_seconds;
                m.score = similarity(s[m.script_index].text, a[j - 1].text);
                out.push_back(m);
            }
            --j;
        } else
            break;
    }
    std::reverse(out.begin(), out.end());
    return out;
}
}  // namespace nd
