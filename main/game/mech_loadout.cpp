#include "mech_loadout.hpp"
#include "run_upgrades.hpp"

namespace Game {

MechLoadout::MechLoadout() = default;

void MechLoadout::resetBonuses() {
    m_bonuses = RunBonuses{};
}

void MechLoadout::applyPreset(const MechLoadoutPreset& preset) {
    resetBonuses();
    setFrame(preset.frame);
    for (uint8_t i = 0; i < static_cast<uint8_t>(MechPartSlot::COUNT); ++i) {
        m_parts[i] = nullptr;
    }
    for (uint8_t i = 0; i < MECH_WEAPON_SLOTS; ++i) {
        m_weapons[i] = nullptr;
    }
    for (uint8_t i = 0; i < preset.partCount; ++i) {
        equipPart(preset.parts[i]);
    }
    equipWeapon(0, preset.primaryWeapon);
    equipWeapon(1, preset.secondaryWeapon);
    m_activeWeaponSlot = 0;
}

void MechLoadout::setFrame(const MechFrameDef* frame) {
    m_frame = frame;
}

bool MechLoadout::equipPart(const MechComponentDef* part) {
    if (!part) return false;
    const uint8_t idx = static_cast<uint8_t>(part->slot);
    if (idx >= static_cast<uint8_t>(MechPartSlot::COUNT)) return false;
    m_parts[idx] = part;
    return true;
}

bool MechLoadout::equipWeapon(int slotIndex, const WeaponDef* weapon) {
    if (slotIndex < 0 || slotIndex >= MECH_WEAPON_SLOTS) return false;
    m_weapons[slotIndex] = weapon;
    return true;
}

bool MechLoadout::upgradePart(MechPartSlot slot) {
    const MechComponentDef* current = part(slot);
    if (!current || !current->upgradeTo) return false;
    m_parts[static_cast<uint8_t>(slot)] = current->upgradeTo;
    return true;
}

bool MechLoadout::upgradeWeapon(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= MECH_WEAPON_SLOTS) return false;
    const WeaponDef* current = m_weapons[slotIndex];
    if (!current || !current->upgradeTo) return false;
    m_weapons[slotIndex] = current->upgradeTo;
    return true;
}

const MechComponentDef* MechLoadout::part(MechPartSlot slot) const {
    const uint8_t idx = static_cast<uint8_t>(slot);
    if (idx >= static_cast<uint8_t>(MechPartSlot::COUNT)) return nullptr;
    return m_parts[idx];
}

const WeaponDef* MechLoadout::weapon(int slotIndex) const {
    if (slotIndex < 0 || slotIndex >= MECH_WEAPON_SLOTS) return nullptr;
    return m_weapons[slotIndex];
}

void MechLoadout::setActiveWeaponSlot(int slotIndex) {
    if (slotIndex >= 0 && slotIndex < MECH_WEAPON_SLOTS && m_weapons[slotIndex]) {
        m_activeWeaponSlot = slotIndex;
    }
}

float MechLoadout::moveSpeed() const {
    float speed = m_frame ? m_frame->baseSpeed : 120.0f;
    for (const MechComponentDef* partDef : m_parts) {
        if (partDef) speed += partDef->stats.speed;
    }
    for (const WeaponDef* w : m_weapons) {
        if (w) speed += w->stats.speed;
    }
    speed += moveBonusForSpeedTier(m_bonuses.speedTier);
    if (speed < 40.0f) speed = 40.0f;
    return speed;
}

float MechLoadout::turnRate() const {
    float turn = m_frame ? m_frame->baseTurnRate : 90.0f;
    for (const MechComponentDef* partDef : m_parts) {
        if (partDef) turn += partDef->stats.turnRate;
    }
    for (const WeaponDef* w : m_weapons) {
        if (w) turn += w->stats.turnRate;
    }
    turn += turnBonusForSpeedTier(m_bonuses.speedTier);
    return turn;
}

float MechLoadout::hitWidth() const {
    float width = m_frame ? m_frame->hitWidth : 28.0f;
    for (const MechComponentDef* partDef : m_parts) {
        if (partDef) width += partDef->stats.hitWidth;
    }
    return width;
}

int MechLoadout::maxHp() const {
    int hp = maxHpForArmorTier(m_bonuses.armorTier);
    for (const MechComponentDef* partDef : m_parts) {
        if (partDef) hp += partDef->stats.hp;
    }
    for (const WeaponDef* w : m_weapons) {
        if (w) hp += w->stats.hp;
    }
    if (hp < 1) hp = 1;
    return hp;
}

int MechLoadout::weaponDamage() const {
    return laserDamageForTier(m_bonuses.laserTier);
}

int8_t MechLoadout::visualPitch() const {
    return m_frame ? m_frame->visualPitch : -8;
}

} // namespace Game
