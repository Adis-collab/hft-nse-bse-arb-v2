#include "common/csv.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace hft {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

std::vector<std::string> split_csv(const std::string& line) {
    std::vector<std::string> cols;
    std::string cur;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            if (in_quotes && i + 1 < line.size() && line[i + 1] == '"') {
                cur.push_back('"');
                ++i;
            } else {
                in_quotes = !in_quotes;
            }
        } else if (c == ',' && !in_quotes) {
            cols.push_back(trim(cur));
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    cols.push_back(trim(cur));
    return cols;
}

bool parse_bool(const std::string& s, bool default_value) {
    std::string v = upper(trim(s));
    if (v == "TRUE" || v == "YES" || v == "1") return true;
    if (v == "FALSE" || v == "NO" || v == "0") return false;
    return default_value;
}

int64_t parse_i64(const std::string& s, int64_t default_value) {
    try {
        if (trim(s).empty()) return default_value;
        return std::stoll(trim(s));
    } catch (...) {
        return default_value;
    }
}

uint64_t parse_u64(const std::string& s, uint64_t default_value) {
    try {
        if (trim(s).empty()) return default_value;
        return static_cast<uint64_t>(std::stoull(trim(s)));
    } catch (...) {
        return default_value;
    }
}

std::unordered_map<std::string, std::string> read_key_value_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("cannot open config file: " + path);
    std::unordered_map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        line = trim(line);
        if (line.empty()) continue;
        auto pos = line.find(':');
        if (pos == std::string::npos) continue;
        std::string key = trim(line.substr(0, pos));
        std::string val = trim(line.substr(pos + 1));
        kv[key] = val;
    }
    return kv;
}

void ensure_parent_dir(const std::string& file_path) {
    std::filesystem::path p(file_path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
}

} // namespace hft
