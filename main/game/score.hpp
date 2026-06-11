#pragma once

#include <cstdint>

namespace Game {

struct RunScore {
    int kills = 0;
    int objectives = 0;
    int killPointValue = 100;
    int objectivePointValue = 500;

    int total() const;
    void reset();
    void setPointValues(int killValue, int objectiveValue);
};

} // namespace Game
