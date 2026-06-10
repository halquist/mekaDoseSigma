#include "score.hpp"

namespace Game {

int RunScore::total() const {
    const int timePts = static_cast<int>(timeSec * 10.0f);
    return kills * 100 + objectives * 500 + timePts;
}

void RunScore::reset() {
    kills = 0;
    objectives = 0;
    timeSec = 0.0f;
}

} // namespace Game
