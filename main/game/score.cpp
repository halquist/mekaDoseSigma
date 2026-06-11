#include "score.hpp"

namespace Game {

int RunScore::total() const {
    return kills * 100 + objectives * 500;
}

void RunScore::reset() {
    kills = 0;
    objectives = 0;
}

} // namespace Game
