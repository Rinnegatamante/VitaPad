#ifndef HEARTBEAT_HPP
#define HEARTBEAT_HPP

#include <array>
#include <cstdint>

const std::array<uint8_t, 6> heartbeat_magic{0x45, 0x4e, 0x44,
                                             0x48, 0x42, 0x54};

#endif // HEARTBEAT_HPP
