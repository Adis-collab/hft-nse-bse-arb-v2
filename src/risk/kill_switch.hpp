#pragma once

#include <string>

namespace hft {

class KillSwitch {
public:
    void disable(std::string reason) { enabled_ = false; reason_ = std::move(reason); }
    void enable() { enabled_ = true; reason_.clear(); }
    bool enabled() const { return enabled_; }
    const std::string& reason() const { return reason_; }
private:
    bool enabled_{true};
    std::string reason_;
};

} // namespace hft
