#pragma once

#include <cstdint>

namespace Game {

struct RunScore {
    int kills = 0;
    int objectives = 0;

    int total() const;
    void reset();
};

} // namespace Game
