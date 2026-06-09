#include "mech_catalog.hpp"
#include "mech_layout_striker_hover.hpp"
#include "mech_layout_striker_mk1.hpp"
#include <cstring>

namespace Game {
namespace MechCatalog {

const MechComponentDef* findComponentById(const char* id) {
    if (!id) return nullptr;
    for (const MechComponentDef* part : STRIKER_HOVER_PARTS) {
        if (part->id && strcmp(part->id, id) == 0) return part;
    }
    for (const MechComponentDef* part : STRIKER_MK1_PARTS) {
        if (part->id && strcmp(part->id, id) == 0) return part;
    }
    return nullptr;
}

const WeaponDef* findWeaponById(const char* id) {
    if (!id) return nullptr;
    if (strcmp(id, WEAPON_homing_missile_mk1.id) == 0) return &WEAPON_homing_missile_mk1;
    if (strcmp(id, WEAPON_homing_missile_mk2.id) == 0) return &WEAPON_homing_missile_mk2;
    if (strcmp(id, WEAPON_rapid_bolt_mk1.id) == 0) return &WEAPON_rapid_bolt_mk1;
    return nullptr;
}

} // namespace MechCatalog
} // namespace Game
