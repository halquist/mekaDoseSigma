#pragma once

#include <cstdint>

namespace Game {

namespace HighScores {

static constexpr int kTopCount = 3;

bool init();
void load(int outScores[kTopCount]);
bool tryAddScore(int score);
int best();

} // namespace HighScores

} // namespace Game
