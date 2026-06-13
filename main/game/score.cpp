#include "score.hpp"

namespace Game {

int RunScore::total() const {
    return kills * killPointValue + objectives * objectivePointValue + bonus;
}

void RunScore::reset() {
    kills = 0;
    objectives = 0;
    bonus = 0;
}

void RunScore::addBonus(int points) {
    if (points > 0) {
        bonus += points;
    }
}

void RunScore::setPointValues(int killValue, int objectiveValue) {
    killPointValue = killValue;
    objectivePointValue = objectiveValue;
}

} // namespace Game
