#pragma once

#include "types.hpp"
#include "driver/i2c.h"

namespace Game {

/**
 * Touch layout for 240px-wide round display (center = width / 2).
 *
 *  |DODGE|  STEER L  | FWD |  STEER R  |DODGE|
 *  0    36         92   148         204   239
 *
 * Outer strips dodge; inner regions turn / move forward.
 */
struct TouchZones {
    static constexpr int FORWARD_HALF_WIDTH = 28;
    static constexpr int DODGE_EDGE_WIDTH = 36;

    static int centerX(int screenWidth) { return screenWidth / 2; }

    static bool isDodgeLeft(int x, int /*screenWidth*/) {
        return x < DODGE_EDGE_WIDTH;
    }

    static bool isDodgeRight(int x, int screenWidth) {
        return x >= screenWidth - DODGE_EDGE_WIDTH;
    }

    static bool isForward(int x, int screenWidth) {
        int c = centerX(screenWidth);
        return x >= c - FORWARD_HALF_WIDTH && x <= c + FORWARD_HALF_WIDTH;
    }

    static bool isSteerLeft(int x, int screenWidth) {
        if (isDodgeLeft(x, screenWidth)) return false;
        int c = centerX(screenWidth);
        return x < c - FORWARD_HALF_WIDTH;
    }

    static bool isSteerRight(int x, int screenWidth) {
        if (isDodgeRight(x, screenWidth)) return false;
        int c = centerX(screenWidth);
        return x > c + FORWARD_HALF_WIDTH;
    }

    static bool isAction(int x, int screenWidth) { return isForward(x, screenWidth); }
};

class TouchInput_t {
public:
    bool init();
    TouchInput read();

private:
    static constexpr uint8_t CHSC6X_ADDR = 0x2e;
    static constexpr uint8_t CHSC6X_READ_LEN = 5;
    static constexpr gpio_num_t I2C_SDA = GPIO_NUM_5;    // D4
    static constexpr gpio_num_t I2C_SCL = GPIO_NUM_6;    // D5
    static constexpr gpio_num_t TOUCH_INT = GPIO_NUM_44; // D7

    bool isPressed();
    bool readBytes(uint8_t* data, size_t len);
};

} // namespace Game
