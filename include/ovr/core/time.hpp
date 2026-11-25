#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>

namespace ovr::core {
using Clock = std::chrono::steady_clock;

inline std::uint64_t now_ns() {
    const auto since_epoch = Clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(since_epoch).count());
}

inline std::uint32_t ms_to_frames(double ms, double hz) {
    if (!(hz > 0.0) || !(ms > 0.0)) {
        return 0;
    }
    const double frames = ms * hz / 1000.0;
    if (!std::isfinite(frames) || frames > static_cast<double>(UINT32_MAX)) {
        return UINT32_MAX;
    }
    return frames < 1.0 ? 1 : static_cast<std::uint32_t>(std::ceil(frames));
}
}
