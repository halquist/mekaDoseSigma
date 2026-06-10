#include "game_ui.hpp"
#include "digits.hpp"
#include "FramebufferIO.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {
namespace GameUi {

namespace {

void fillCircle(uint16_t* framebuffer, int width, int height,
                int cx, int cy, int radius, uint16_t color) {
    const uint16_t packed = fbPack(color);
    const int r2 = radius * radius;
    for (int y = -radius; y <= radius; ++y) {
        const int py = cy + y;
        if (py < 0 || py >= height) {
            continue;
        }
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y > r2) {
                continue;
            }
            const int px = cx + x;
            if (px < 0 || px >= width) {
                continue;
            }
            framebuffer[py * width + px] = packed;
        }
    }
}

void drawTag(uint16_t* framebuffer, int width, int height,
             int cx, int cy, const char* tag, uint16_t color, int scale) {
    const uint16_t packed = fbPack(color);
    const int tagLen = 3;
    const int charW = 4 * scale;
    const int spacing = scale;
    const int totalW = tagLen * charW + (tagLen - 1) * spacing;
    int x = cx - totalW / 2;

    for (int i = 0; tag[i] != '\0' && i < tagLen; ++i) {
        const char ch = tag[i];
        if (ch >= '0' && ch <= '9') {
            Digits::drawDigit(framebuffer, width, height, ch - '0',
                              x + charW / 2, cy, scale, color);
        } else if (ch == '+') {
            for (int sy = 0; sy < scale; ++sy) {
                const int py = cy + sy - scale / 2;
                if (py >= 0 && py < height) {
                    for (int sx = -scale; sx <= scale; ++sx) {
                        const int px = x + charW / 2 + sx;
                        if (px >= 0 && px < width) {
                            framebuffer[py * width + px] = packed;
                        }
                    }
                }
            }
            for (int sx = 0; sx < scale; ++sx) {
                const int px = x + charW / 2 + sx - scale / 2;
                if (px >= 0 && px < width) {
                    for (int sy = -scale; sy <= scale; ++sy) {
                        const int py = cy + sy;
                        if (py >= 0 && py < height) {
                            framebuffer[py * width + px] = packed;
                        }
                    }
                }
            }
        } else {
            for (int sy = 0; sy < 3 * scale; ++sy) {
                const int py = cy - scale + sy;
                if (py < 0 || py >= height) {
                    continue;
                }
                for (int sx = 0; sx < 2 * scale; ++sx) {
                    const int px = x + sx;
                    if (px >= 0 && px < width) {
                        framebuffer[py * width + px] = packed;
                    }
                }
            }
        }
        x += charW + spacing;
    }
}

void drawDimVignette(uint16_t* framebuffer, int width, int height, uint16_t tint) {
    const float cx = width * 0.5f;
    const float cy = height * 0.5f;
    const float maxR = (cx < cy) ? cx : cy;
    const uint16_t packedTint = fbPack(tint);

    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {
            const float dx = static_cast<float>(px) + 0.5f - cx;
            const float dy = static_cast<float>(py) + 0.5f - cy;
            const float dist = sqrtf(dx * dx + dy * dy);
            if (dist > maxR * 0.35f) {
                continue;
            }
            const float alpha = 0.45f * (1.0f - dist / (maxR * 0.95f));
            if (alpha <= 0.02f) {
                continue;
            }
            const uint16_t base = framebuffer[py * width + px];
            const uint8_t br = (base >> 11) & 0x1F;
            const uint8_t bg = (base >> 5) & 0x3F;
            const uint8_t bb = base & 0x1F;
            const uint8_t tr = (packedTint >> 11) & 0x1F;
            const uint8_t tg = (packedTint >> 5) & 0x3F;
            const uint8_t tb = packedTint & 0x1F;
            const uint8_t nr = static_cast<uint8_t>(br + (tr - br) * alpha);
            const uint8_t ng = static_cast<uint8_t>(bg + (tg - bg) * alpha);
            const uint8_t nb = static_cast<uint8_t>(bb + (tb - bb) * alpha);
            framebuffer[py * width + px] =
                static_cast<uint16_t>((nr << 11) | (ng << 5) | nb);
        }
    }
}

} // namespace

void drawMenu(uint16_t* framebuffer, int width, int height, int bestScore, float pulseSec) {
    drawDimVignette(framebuffer, width, height, Colors::HUD_BG);

    const int cx = width / 2;
    const int cy = height / 2 - 18;
    fillCircle(framebuffer, width, height, cx, cy, 34, Colors::MECH_BLUE);
    fillCircle(framebuffer, width, height, cx, cy, 22, Colors::MECH_WHITE);

    Digits::drawNumber(framebuffer, width, height, bestScore,
                       width / 2 + 36, 28, 2, Colors::HUD_TEXT);

    const bool pulseOn = fmodf(pulseSec, 1.2f) < 0.7f;
    if (pulseOn) {
        Digits::drawNumber(framebuffer, width, height, 1,
                           width / 2 + 8, height - 28, 2, Colors::OBJECTIVE_ARROW);
    }
}

void drawDefeat(uint16_t* framebuffer, int width, int height,
                const RunScore& score, int bestScore, bool newHighScore) {
    drawDimVignette(framebuffer, width, height, Colors::DAMAGE_RED);

    const int total = score.total();
    Digits::drawNumber(framebuffer, width, height, total,
                       width / 2 + 42, height / 2 - 8, 2, Colors::HUD_TEXT);

    const int shownBest = newHighScore ? total : bestScore;
    Digits::drawNumber(framebuffer, width, height, shownBest,
                       width / 2 + 36, 36, 2,
                       newHighScore ? Colors::VICTORY_GREEN : Colors::HUD_TEXT);

    if (newHighScore) {
        fillCircle(framebuffer, width, height, width / 2, height / 2 + 34, 8,
                   Colors::VICTORY_GREEN);
    }
}

void drawUpgradePick(uint16_t* framebuffer, int width, int height,
                     const UpgradeOption& left, const UpgradeOption& right) {
    drawDimVignette(framebuffer, width, height, Colors::OBJECTIVE_DOME);

    const int cy = height / 2 + 24;
    const int leftCx = width / 4;
    const int rightCx = (width * 3) / 4;

    fillCircle(framebuffer, width, height, leftCx, cy, 42, Colors::HUD_BG);
    fillCircle(framebuffer, width, height, rightCx, cy, 42, Colors::HUD_BG);
    fillCircle(framebuffer, width, height, leftCx, cy, 34, left.color);
    fillCircle(framebuffer, width, height, rightCx, cy, 34, right.color);

    drawTag(framebuffer, width, height, leftCx, cy + 4, left.tag, Colors::HUD_TEXT, 2);
    drawTag(framebuffer, width, height, rightCx, cy + 4, right.tag, Colors::HUD_TEXT, 2);
}

int upgradePickFromTouch(int touchX, int touchY, int width, int height) {
    if (touchY < height / 2) {
        return -1;
    }
    if (touchX < width / 2) {
        return 0;
    }
    return 1;
}

} // namespace GameUi
} // namespace Game
