#include "score.hpp"

namespace Game {

int RunScore::total() const {
    return kills * killPointValue + objectives * objectivePointValue;
}

void RunScore::reset() {
    kills = 0;
    objectives = 0;
}

void RunScore::setPointValues(int killValue, int objectiveValue) {
    killPointValue = killValue;
    objectivePointValue = objectiveValue;
}

} // namespace Game
