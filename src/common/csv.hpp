#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace hft {

std::string trim(const std::string& s);
std::string upper(std::string s);
std::vector<std::string> split_csv(const std::string& line);
bool parse_bool(const std::string& s, bool default_value = false);
int64_t parse_i64(const std::string& s, int64_t default_value = 0);
uint64_t parse_u64(const std::string& s, uint64_t default_value = 0);
std::unordered_map<std::string, std::string> read_key_value_file(const std::string& path);
void ensure_parent_dir(const std::string& file_path);

} // namespace hft
