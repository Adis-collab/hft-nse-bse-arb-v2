#pragma once

#include <fstream>
#include <optional>
#include <string>
#include "md/packet.hpp"

namespace hft {

class ReplayReader {
public:
    explicit ReplayReader(std::string path);
    std::optional<RawPacket> next_packet();
    bool good() const { return in_.good(); }

private:
    std::ifstream in_;
    std::string path_;
    bool header_skipped_{false};
};

} // namespace hft
