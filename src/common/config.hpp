#pragma once
#include <string>
#include <unordered_map>

namespace hft {

class SimpleConfig {
public:
    static SimpleConfig load(const std::string& path);
    std::string get_string(const std::string& key, const std::string& def = "") const;
    int get_int(const std::string& key, int def = 0) const;
    long long get_i64(const std::string& key, long long def = 0) const;
    bool get_bool(const std::string& key, bool def = false) const;
private:
    std::unordered_map<std::string, std::string> kv_;
};

} // namespace hft
