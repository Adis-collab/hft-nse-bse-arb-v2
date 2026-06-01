#include "common/config.hpp"
#include "common/csv.hpp"

namespace hft {

SimpleConfig SimpleConfig::load(const std::string& path) {
    SimpleConfig c;
    c.kv_ = read_key_value_file(path);
    return c;
}

std::string SimpleConfig::get_string(const std::string& key, const std::string& def) const {
    auto it = kv_.find(key);
    return it == kv_.end() ? def : it->second;
}

int SimpleConfig::get_int(const std::string& key, int def) const {
    auto it = kv_.find(key);
    return it == kv_.end() ? def : static_cast<int>(parse_i64(it->second, def));
}

long long SimpleConfig::get_i64(const std::string& key, long long def) const {
    auto it = kv_.find(key);
    return it == kv_.end() ? def : parse_i64(it->second, def);
}

bool SimpleConfig::get_bool(const std::string& key, bool def) const {
    auto it = kv_.find(key);
    return it == kv_.end() ? def : parse_bool(it->second, def);
}

} // namespace hft
