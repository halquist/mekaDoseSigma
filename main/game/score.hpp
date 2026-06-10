#pragma once

#include <cstdint>

namespace Game {

struct RunScore {
    int kills = 0;
    int objectives = 0;
    float timeSec = 0.0f;

    int total() const;
    void reset();
};

} // namespace Game
