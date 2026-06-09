#pragma once

#include "mech_config.hpp"

namespace Game {

/// Runtime equipped frame, components, and weapons with stat aggregation.
class MechLoadout {
public:
    MechLoadout();

    void applyPreset(const MechLoadoutPreset& preset);
    void setFrame(const MechFrameDef* frame);

    bool equipPart(const MechComponentDef* part);
    bool equipWeapon(int slotIndex, const WeaponDef* weapon);
    bool upgradePart(MechPartSlot slot);
    bool upgradeWeapon(int slotIndex);

    const MechFrameDef* frame() const { return m_frame; }
    const MechComponentDef* part(MechPartSlot slot) const;
    const WeaponDef* weapon(int slotIndex) const;
    int activeWeaponSlot() const { return m_activeWeaponSlot; }
    void setActiveWeaponSlot(int slotIndex);

    float moveSpeed() const;
    float turnRate() const;
    float hitWidth() const;
    int maxHp() const;
    int8_t visualPitch() const;

private:
    const MechFrameDef* m_frame = nullptr;
    const MechComponentDef* m_parts[static_cast<uint8_t>(MechPartSlot::COUNT)] = {};
    const WeaponDef* m_weapons[MECH_WEAPON_SLOTS] = {};
    int m_activeWeaponSlot = 0;
};

} // namespace Game
