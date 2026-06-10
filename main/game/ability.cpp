#include "ability.hpp"
#include "scene_util.hpp"
#include <cmath>

namespace Game {

namespace {

float smoothStep(float t) {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

uint16_t brightenRgb565(uint16_t color) {
    const uint8_t r = ((color >> 11) & 0x1F) << 3;
    const uint8_t g = ((color >> 5) & 0x3F) << 2;
    const uint8_t b = (color & 0x1F) << 3;
    const uint8_t nr = (r + (r >> 2) > 255) ? 255 : static_cast<uint8_t>(r + (r >> 2));
    const uint8_t ng = (g + (g >> 2) > 255) ? 255 : static_cast<uint8_t>(g + (g >> 2));
    const uint8_t nb = (b + (b >> 2) > 255) ? 255 : static_cast<uint8_t>(b + (b >> 2));
    return Colors::rgb(nr, ng, nb);
}

} // namespace

namespace AbilityCatalog {

const AbilityDef SHIELD = {
    AbilityKind::Shield,
    50,
    8.0f,
};

const AbilityDef SHIELD_ENEMY = {
    AbilityKind::Shield,
    24,
    0.0f,
};

} // namespace AbilityCatalog

MechAbility::MechAbility(Renderer::Scene& scene)
    : m_scene(scene)
    , m_shieldMat(Colors::SHIELD_BLUE, SHIELD_PEAK_ALPHA)
{
    m_shieldMat.shadingMode = Renderer::ShadingMode::UNLIT;
}

void MechAbility::equip(const AbilityDef& def) {
    m_def = def;
    m_maxShieldHp = def.shieldCapacity;
    m_singleUse = false;
    reset();
}

void MechAbility::setShieldColor(uint16_t color, uint8_t alpha) {
    m_shieldColorBase = color;
    m_shieldMat.color = color;
    m_shieldMat.alpha = alpha;
}

void MechAbility::reset() {
    m_state = State::Ready;
    m_visualPhase = VisualPhase::Off;
    m_shieldHp = 0;
    m_cooldownRemaining = 0.0f;
    m_lastCenterTapTime = -100.0f;
    m_animTimer = 0.0f;
    showShieldMesh(false);
}

void MechAbility::ensureShieldMesh() {
    if (m_shieldObj) {
        return;
    }

    m_shieldObj = Primitives::createSphere(SHIELD_RADIUS, SHIELD_SPHERE_SEGMENTS,
                                           &m_shieldMat);
    m_shieldBaseVerts.clear();
    m_shieldBaseVerts.reserve(m_shieldObj->vertices.size());
    for (const auto& v : m_shieldObj->vertices) {
        m_shieldBaseVerts.push_back({v.position.x, v.position.y, v.position.z});
    }

    m_shieldObj->zBias = 0;
    m_scene.addObject(m_shieldObj);
    stashSceneObject(m_shieldObj);
}

void MechAbility::showShieldMesh(bool visible) {
    if (!m_shieldObj) {
        if (!visible) {
            return;
        }
        ensureShieldMesh();
    }

    if (visible) {
        enableSceneObject(m_shieldObj);
    } else {
        stashSceneObject(m_shieldObj);
    }
}

void MechAbility::applyShieldScale(float scale) {
    if (!m_shieldObj || m_shieldBaseVerts.empty()) {
        return;
    }

    const size_t count = m_shieldBaseVerts.size();
    for (size_t i = 0; i < count; ++i) {
        auto& pos = m_shieldObj->vertices[i].position;
        pos.x = static_cast<int32_t>(lroundf(static_cast<float>(m_shieldBaseVerts[i].x) * scale));
        pos.y = static_cast<int32_t>(lroundf(static_cast<float>(m_shieldBaseVerts[i].y) * scale));
        pos.z = static_cast<int32_t>(lroundf(static_cast<float>(m_shieldBaseVerts[i].z) * scale));
    }
    m_shieldObj->calculateBoundingBox();
}

void MechAbility::updateShieldVisual(float deltaTime, int32_t x, int32_t z, float baseY) {
    if (m_visualPhase == VisualPhase::Off) {
        return;
    }

    ensureShieldMesh();
    m_animTimer += deltaTime;

    const int32_t cy = static_cast<int32_t>(lroundf(baseY + SHIELD_CENTER_Y));
    m_shieldObj->setPosition(x, cy, z);
    m_shieldObj->setRotation(0, 0, 0);

    switch (m_visualPhase) {
        case VisualPhase::DeployGrow: {
            const float t = smoothStep(m_animTimer / DEPLOY_GROW_SEC);
            const float scale = SHIELD_SCALE_START + (1.0f - SHIELD_SCALE_START) * t;
            m_shieldMat.color = m_shieldColorBase;
            m_shieldMat.alpha = static_cast<uint8_t>(SHIELD_PEAK_ALPHA * t);
            applyShieldScale(scale);
            showShieldMesh(m_shieldMat.alpha > 0);
            if (m_animTimer >= DEPLOY_GROW_SEC) {
                m_animTimer = 0.0f;
                m_visualPhase = VisualPhase::DeployFade;
            }
            break;
        }
        case VisualPhase::DeployFade: {
            const float t = smoothStep(m_animTimer / DEPLOY_FADE_SEC);
            m_shieldMat.color = m_shieldColorBase;
            m_shieldMat.alpha = static_cast<uint8_t>(SHIELD_PEAK_ALPHA * (1.0f - t));
            applyShieldScale(1.0f);
            showShieldMesh(m_shieldMat.alpha > 0);
            if (m_animTimer >= DEPLOY_FADE_SEC) {
                showShieldMesh(false);
                m_visualPhase = VisualPhase::Off;
                m_animTimer = 0.0f;
            }
            break;
        }
        case VisualPhase::BreakAnim: {
            const float t = smoothStep(m_animTimer / BREAK_ANIM_SEC);
            const float scale = 1.0f + (BREAK_SCALE_END - 1.0f) * t;
            m_shieldMat.color = brightenRgb565(m_shieldColorBase);
            m_shieldMat.alpha = static_cast<uint8_t>(SHIELD_BREAK_ALPHA * (1.0f - t));
            applyShieldScale(scale);
            showShieldMesh(m_shieldMat.alpha > 0);
            if (m_animTimer >= BREAK_ANIM_SEC) {
                finishBreakVisual();
            }
            break;
        }
        default:
            break;
    }
}

void MechAbility::startDeployVisual() {
    m_visualPhase = VisualPhase::DeployGrow;
    m_animTimer = 0.0f;
    ensureShieldMesh();
    m_shieldMat.color = m_shieldColorBase;
    m_shieldMat.alpha = 0;
    applyShieldScale(SHIELD_SCALE_START);
    showShieldMesh(true);
}

void MechAbility::startBreakVisual() {
    m_visualPhase = VisualPhase::BreakAnim;
    m_animTimer = 0.0f;
    ensureShieldMesh();
    applyShieldScale(1.0f);
    m_shieldMat.color = brightenRgb565(m_shieldColorBase);
    m_shieldMat.alpha = SHIELD_BREAK_ALPHA;
    showShieldMesh(true);
}

void MechAbility::finishBreakVisual() {
    showShieldMesh(false);
    m_visualPhase = VisualPhase::Off;
    m_animTimer = 0.0f;
    if (m_singleUse) {
        m_state = State::Spent;
    } else {
        beginCooldown();
    }
}

void MechAbility::update(float deltaTime, const MechRig& rig,
                         int32_t x, int32_t z, float baseY,
                         int32_t angle, int8_t pitch) {
    (void)rig;
    (void)angle;
    (void)pitch;

    if (m_state == State::Cooldown) {
        m_cooldownRemaining -= deltaTime;
        if (m_cooldownRemaining <= 0.0f) {
            m_cooldownRemaining = 0.0f;
            m_state = State::Ready;
        }
    }

    if (m_state == State::Active || m_visualPhase == VisualPhase::BreakAnim) {
        updateShieldVisual(deltaTime, x, z, baseY);
    }
}

bool MechAbility::onCenterTap(float nowSec) {
    if (m_lastCenterTapTime >= 0.0f &&
        (nowSec - m_lastCenterTapTime) <= DOUBLE_TAP_SEC) {
        m_lastCenterTapTime = -100.0f;
        return tryActivate();
    }

    m_lastCenterTapTime = nowSec;
    return false;
}

bool MechAbility::tryActivate() {
    if (m_def.kind != AbilityKind::Shield || m_state != State::Ready) {
        return false;
    }

    m_state = State::Active;
    m_shieldHp = m_maxShieldHp;
    startDeployVisual();
    return true;
}

void MechAbility::breakShield() {
    m_shieldHp = 0;
    startBreakVisual();
}

void MechAbility::beginCooldown() {
    m_state = State::Cooldown;
    m_cooldownRemaining = m_def.cooldownSec;
}

int MechAbility::absorbDamage(int damage) {
    if (damage <= 0 || m_state != State::Active || m_shieldHp <= 0) {
        return damage;
    }

    if (damage >= m_shieldHp) {
        const int overflow = damage - m_shieldHp;
        breakShield();
        return overflow;
    }

    m_shieldHp -= damage;
    return 0;
}

float MechAbility::cooldownRemaining() const {
    return m_cooldownRemaining;
}

} // namespace Game
