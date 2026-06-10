#pragma once

#include "Object.hpp"

namespace Game {

inline void disableSceneObject(Renderer::Object* obj) {
    if (!obj) return;
    obj->enabled = false;
}

inline void enableSceneObject(Renderer::Object* obj) {
    if (!obj) return;
    obj->enabled = true;
}

inline void stashSceneObject(Renderer::Object* obj) {
    if (!obj) return;
    obj->enabled = false;
    obj->setPosition(0, -1000, 0);
}

inline void showSceneObject(Renderer::Object* obj, int32_t x, int32_t y, int32_t z) {
    if (!obj) return;
    obj->enabled = true;
    obj->setPosition(x, y, z);
}

} // namespace Game
