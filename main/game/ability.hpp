#pragma once

#include "Jet.hpp"
#include "mech_rig.hpp"
#include "types.hpp"
#include <cstdint>
#include <vector>

namespace Game {

enum class AbilityKind : uint8_t {
    None,
    Shield,
};

struct AbilityDef {
    AbilityKind kind = AbilityKind::None;
    int shieldCapacity = 0;
    float cooldownSec = 0.0f;
};

namespace AbilityCatalog {
    extern const AbilityDef SHIELD;
    extern const AbilityDef SHIELD_ENEMY;
}

struct ShieldDamageResult {
    int healthDamage = 0;
    bool hitShield = false;
};

/// Runtime ability state with optional scene visual (shield sphere).
class MechAbility {
public:
    explicit MechAbility(Renderer::Scene& scene);

    void equip(const AbilityDef& def);
    void reset();

    void update(float deltaTime, const MechRig& rig,
                int32_t x, int32_t z, float baseY, int32_t angle, int8_t pitch);

    /// Call on each new touch in the forward/action zone; returns true if activated.
    bool onCenterTap(float nowSec);

    bool tryActivate();

    void setShieldColor(uint16_t color, uint8_t alpha = 120);
    void setSingleUse(bool singleUse) { m_singleUse = singleUse; }

    /// Absorbs damage with the shield when active. No overflow on shield break.
    ShieldDamageResult absorbDamage(int damage);

    bool isReady() const { return m_state == State::Ready; }
    bool isActive() const { return m_state == State::Active; }
    bool showReadyIcon() const { return isReady() && m_def.kind != AbilityKind::None; }
    bool showShieldBar() const { return isActive() && m_shieldHp > 0; }

    int shieldHp() const { return m_shieldHp; }
    int maxShieldHp() const { return m_maxShieldHp; }
    float cooldownRemaining() const;

    /// World position on the shield sphere facing an incoming hit.
    static void shieldHitPosition(float mechX, float baseY, float mechZ,
                                  float hitX, float hitY, float hitZ,
                                  float& outX, float& outY, float& outZ);

private:
    enum class State : uint8_t {
        Ready,
        Active,
        Cooldown,
        Spent,
    };

    enum class VisualPhase : uint8_t {
        Off,
        DeployGrow,
        DeployFade,
        BreakAnim,
    };

    struct BaseVert {
        int32_t x = 0;
        int32_t y = 0;
        int32_t z = 0;
    };

    void ensureShieldMesh();
    void showShieldMesh(bool visible);
    void applyShieldScale(float scale);
    void updateShieldVisual(float deltaTime, int32_t x, int32_t z, float baseY);
    void startDeployVisual();
    void startBreakVisual();
    void finishBreakVisual();
    void breakShield();
    void beginCooldown();

    Renderer::Scene& m_scene;
    AbilityDef m_def{};
    State m_state = State::Ready;
    VisualPhase m_visualPhase = VisualPhase::Off;

    Renderer::Object* m_shieldObj = nullptr;
    Renderer::Material m_shieldMat;
    std::vector<BaseVert> m_shieldBaseVerts;
    uint16_t m_shieldColorBase = Colors::SHIELD_BLUE;

    int m_shieldHp = 0;
    int m_maxShieldHp = 0;
    float m_cooldownRemaining = 0.0f;
    float m_lastCenterTapTime = -100.0f;
    float m_animTimer = 0.0f;
    bool m_singleUse = false;

    static constexpr float DOUBLE_TAP_SEC = 0.35f;
    static constexpr int32_t SHIELD_RADIUS = 40;
    static constexpr int SHIELD_SPHERE_SEGMENTS = 6;
    static constexpr float SHIELD_CENTER_Y = 30.0f;
    static constexpr float SHIELD_SCALE_START = 0.12f;
    static constexpr float DEPLOY_GROW_SEC = 0.7f;
    static constexpr float DEPLOY_FADE_SEC = 0.7f;
    static constexpr float BREAK_ANIM_SEC = 0.7f;
    static constexpr float BREAK_SCALE_END = 1.35f;
    static constexpr uint8_t SHIELD_PEAK_ALPHA = 130;
    static constexpr uint8_t SHIELD_BREAK_ALPHA = 200;
};

} // namespace Game
