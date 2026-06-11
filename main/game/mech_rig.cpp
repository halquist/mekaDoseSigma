#include "mech_rig.hpp"
#include "types.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

struct Mat3 {
    float m[3][3];
};

Mat3 matIdentity() {
    return {{{1.0f, 0.0f, 0.0f},
             {0.0f, 1.0f, 0.0f},
             {0.0f, 0.0f, 1.0f}}};
}

Mat3 matRx(float deg) {
    const float r = deg * static_cast<float>(M_PI) / 180.0f;
    const float c = cosf(r);
    const float s = sinf(r);
    return {{{1.0f, 0.0f, 0.0f},
             {0.0f, c, -s},
             {0.0f, s, c}}};
}

Mat3 matRy(float deg) {
    const float r = deg * static_cast<float>(M_PI) / 180.0f;
    const float c = cosf(r);
    const float s = sinf(r);
    return {{{c, 0.0f, s},
             {0.0f, 1.0f, 0.0f},
             {-s, 0.0f, c}}};
}

Mat3 matRz(float deg) {
    const float r = deg * static_cast<float>(M_PI) / 180.0f;
    const float c = cosf(r);
    const float s = sinf(r);
    return {{{c, -s, 0.0f},
             {s, c, 0.0f},
             {0.0f, 0.0f, 1.0f}}};
}

Mat3 matMul(const Mat3& a, const Mat3& b) {
    Mat3 out = matIdentity();
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            out.m[row][col] =
                a.m[row][0] * b.m[0][col] +
                a.m[row][1] * b.m[1][col] +
                a.m[row][2] * b.m[2][col];
        }
    }
    return out;
}

Mat3 partLocalRotation(const MechComponentDef& def) {
    return matMul(matRz(static_cast<float>(def.localRotZ)),
                  matMul(matRy(static_cast<float>(def.localRotY)),
                         matRx(static_cast<float>(def.localRotX))));
}

void transformPosition(Mat3 rot, float tx, float ty, float tz,
                       int32_t& x, int32_t& y, int32_t& z) {
    const float px = static_cast<float>(x);
    const float py = static_cast<float>(y);
    const float pz = static_cast<float>(z);
    const float ox = rot.m[0][0] * px + rot.m[0][1] * py + rot.m[0][2] * pz + tx;
    const float oy = rot.m[1][0] * px + rot.m[1][1] * py + rot.m[1][2] * pz + ty;
    const float oz = rot.m[2][0] * px + rot.m[2][1] * py + rot.m[2][2] * pz + tz;
    x = static_cast<int32_t>(lroundf(ox));
    y = static_cast<int32_t>(lroundf(oy));
    z = static_cast<int32_t>(lroundf(oz));
}

void transformNormal(Mat3 rot, int32_t& x, int32_t& y, int32_t& z) {
    const float inv = 1.0f / static_cast<float>(FIXED_POINT_SCALE);
    float nx = x * inv;
    float ny = y * inv;
    float nz = z * inv;
    const float ox = rot.m[0][0] * nx + rot.m[0][1] * ny + rot.m[0][2] * nz;
    const float oy = rot.m[1][0] * nx + rot.m[1][1] * ny + rot.m[1][2] * nz;
    const float oz = rot.m[2][0] * nx + rot.m[2][1] * ny + rot.m[2][2] * nz;
    const float len = sqrtf(ox * ox + oy * oy + oz * oz);
    if (len > 0.001f) {
        nx = ox / len;
        ny = oy / len;
        nz = oz / len;
    }
    x = static_cast<int32_t>(lroundf(nx * static_cast<float>(FIXED_POINT_SCALE)));
    y = static_cast<int32_t>(lroundf(ny * static_cast<float>(FIXED_POINT_SCALE)));
    z = static_cast<int32_t>(lroundf(nz * static_cast<float>(FIXED_POINT_SCALE)));
}

void centerMeshAtOrigin(Renderer::Object* obj) {
    if (!obj || obj->vertices.empty()) return;

    obj->calculateBoundingBox();
    const int32_t cx = obj->centreVolume.x;
    const int32_t cy = obj->centreVolume.y;
    const int32_t cz = obj->centreVolume.z;
    for (auto& v : obj->vertices) {
        v.position.x -= cx;
        v.position.y -= cy;
        v.position.z -= cz;
    }
    obj->calculateBoundingBox();
}

uint16_t enemyPaletteColor(uint16_t source) {
    using namespace Colors;
    if (source == MECH_WHITE) return rgb(165, 35, 40);
    if (source == MECH_BLUE) return rgb(110, 22, 28);
    if (source == MECH_RED) return rgb(225, 55, 58);
    if (source == TRACER_YELLOW) return rgb(190, 45, 48);
    if (source == MISSILE_GREY) return rgb(150, 38, 42);
    return ENEMY_BODY;
}

uint16_t bossPaletteColor(uint16_t source) {
    using namespace Colors;
    if (source == MECH_WHITE) return rgb(175, 195, 230);
    if (source == MECH_BLUE) return rgb(45, 85, 210);
    if (source == MECH_RED) return rgb(70, 110, 200);
    if (source == TRACER_YELLOW) return rgb(90, 140, 220);
    if (source == MISSILE_GREY) return rgb(100, 130, 190);
    return MECH_BLUE;
}

} // namespace

MechRig::MechRig(Renderer::Scene& scene)
    : m_scene(scene)
{
}

Renderer::Object* MechRig::createPartMesh(const MechComponentDef& def,
                                          Renderer::Material* mat) {
    mat->shadingMode = Renderer::ShadingMode::GOURAUD;
    Renderer::Object* obj = nullptr;
    switch (def.primitive) {
    case MechPrimitiveKind::Cube:
        obj = Primitives::createCube(def.dimA, def.dimB, def.dimC, mat);
        break;
    case MechPrimitiveKind::Pyramid:
        obj = Primitives::createPyramid(def.dimA, def.dimB, mat);
        break;
    case MechPrimitiveKind::Sphere: {
        // Jet spheres need >=6 segments to read round; 3 looks like a bipyramid.
        int32_t segs = def.dimB >= 3 ? def.dimB : 6;
        if (segs > 12) segs = 12;
        obj = Primitives::createSphere(def.dimA, segs, mat);
        break;
    }
    case MechPrimitiveKind::Capsule:
        obj = Primitives::createCapsule(def.dimA, def.dimB, 3, mat);
        break;
    case MechPrimitiveKind::Cylinder:
        obj = Primitives::createCylinder(def.dimA, def.dimB, 3, true, mat);
        break;
    default:
        obj = Primitives::createCube(4, 4, 4, mat);
        break;
    }
    centerMeshAtOrigin(obj);
    return obj;
}

void MechRig::hideBodyObject() {
    if (m_bodyObj) {
        m_bodyObj->enabled = false;
        m_bodyObj->setPosition(0, -1000, 0);
    }
}

void MechRig::ensureBuilt(const MechLoadout& loadout, MechPalette palette) {
    if (m_bodyBuilt) {
        return;
    }
    rebuild(loadout, palette);
}

void MechRig::appendPartToBody(Renderer::Object& body, const MechComponentDef& def,
                               Renderer::Material* mat) {
    Renderer::Object* part = createPartMesh(def, mat);
    if (!part) return;

    const Mat3 rot = partLocalRotation(def);
    const uint16_t baseIndex = static_cast<uint16_t>(body.vertices.size());

    for (auto& v : part->vertices) {
        transformPosition(rot, def.localX, def.localY, def.localZ,
                          v.position.x, v.position.y, v.position.z);
        transformNormal(rot, v.normal.x, v.normal.y, v.normal.z);
        body.addVertex(v);
    }

    for (const auto& tri : part->triangles) {
        body.addTriangle(static_cast<uint16_t>(baseIndex + tri.v1),
                         static_cast<uint16_t>(baseIndex + tri.v2),
                         static_cast<uint16_t>(baseIndex + tri.v3),
                         tri.material);
    }

    delete part;
}

void MechRig::rebuild(const MechLoadout& loadout, MechPalette palette) {
    hideBodyObject();

    for (auto& slotMat : m_partMats) {
        slotMat.active = false;
    }

    Renderer::Object* body = new Renderer::Object();
    body->cullingMode = Renderer::CullingMode::NO_CULLING;

    for (uint8_t i = 0; i < static_cast<uint8_t>(MechPartSlot::COUNT); ++i) {
        const MechPartSlot slot = static_cast<MechPartSlot>(i);
        const MechComponentDef* def = loadout.part(slot);
        if (!def) continue;

        PartMaterial& slotMat = m_partMats[i];
        uint16_t color = def->color;
        if (palette == MechPalette::EnemyRed) {
            color = enemyPaletteColor(def->color);
        } else if (palette == MechPalette::BossBlue) {
            color = bossPaletteColor(def->color);
        }
        slotMat.mat = Renderer::Material(color);
        slotMat.mat.shadingMode = Renderer::ShadingMode::GOURAUD;
        slotMat.active = true;
        appendPartToBody(*body, *def, &slotMat.mat);
    }

    if (body->vertices.empty()) {
        delete body;
        m_bodyObj = nullptr;
        m_bodyBuilt = false;
        return;
    }

    body->calculateBoundingBox();
    m_scene.addObject(body);
    m_bodyObj = body;
    m_bodyBuilt = true;
    m_hidden = false;
}

void MechRig::transformLocalToWorld(float bodyX, float bodyZ, float baseY,
                                    float angle, int8_t pitch,
                                    float localX, float localY, float localZ,
                                    float& worldX, float& worldY,
                                    float& worldZ) const {
    const float pitchRad = pitch * M_PI / 180.0f;
    const float yaw = angle * M_PI / 180.0f;
    const float cx = cosf(pitchRad), sx = sinf(pitchRad);
    const float cy = cosf(yaw), sy = sinf(yaw);

    const float kx = localX;
    const float ky = localY * cx - localZ * sx;
    const float kz = localY * sx + localZ * cx;

    worldX = bodyX + kx * cy + kz * sy;
    worldY = baseY + ky;
    worldZ = bodyZ + -kx * sy + kz * cy;
}

void MechRig::updatePose(int32_t x, int32_t z, float baseY, int32_t angle, int8_t pitch,
                         const MechLoadout& loadout) {
    (void)loadout;
    if (!m_bodyBuilt || !m_bodyObj || m_hidden) return;

    m_bodyObj->setPosition(x, static_cast<int32_t>(lroundf(baseY)), z);
    m_bodyObj->setRotation(pitch, angle, 0);
}

void MechRig::setHidden(bool hidden) {
    m_hidden = hidden;
    if (!m_bodyObj) return;
    if (hidden) {
        hideBodyObject();
    } else {
        m_bodyObj->enabled = true;
    }
}

void MechRig::weaponMuzzleLocal(const WeaponDef& weapon,
                                float& lx, float& ly, float& lz) const {
    lx = weapon.muzzleLocalX;
    ly = weapon.muzzleLocalY;
    lz = weapon.muzzleLocalZ;
}

} // namespace Game
