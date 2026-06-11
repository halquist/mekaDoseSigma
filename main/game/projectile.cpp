#include "projectile.hpp"
#include "scene_util.hpp"
#include "terrain.hpp"
#include "rng.hpp"
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {

namespace {

float distPointToSegment(float px, float pz,
                         float ax, float az, float bx, float bz) {
    float segDx = bx - ax;
    float segDz = bz - az;
    float segLen2 = segDx * segDx + segDz * segDz;
    if (segLen2 < 0.01f) {
        float dx = px - ax;
        float dz = pz - az;
        return sqrtf(dx * dx + dz * dz);
    }
    float t = ((px - ax) * segDx + (pz - az) * segDz) / segLen2;
    if (t < 0.0f) t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    float cx = ax + t * segDx;
    float cz = az + t * segDz;
    float dx = px - cx;
    float dz = pz - cz;
    return sqrtf(dx * dx + dz * dz);
}

float wrapAngleDeg(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

} // namespace

ProjectileSystem::ProjectileSystem(Renderer::Scene& scene)
    : m_scene(scene)
    , m_missileMat(Colors::MISSILE_GREY)
    , m_enemyProjMat(Colors::ORANGE)
    , m_bombMat(Colors::BOMB_BODY)
{
    m_missileMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_enemyProjMat.shadingMode = Renderer::ShadingMode::UNLIT;
    m_bombMat.shadingMode = Renderer::ShadingMode::UNLIT;

    for (auto& p : m_projectiles) {
        p.obj = nullptr;
        p.active = false;
    }

    for (size_t i = 0; i < m_trailPuffs.size(); ++i) {
        auto& t = m_trailPuffs[i];
        m_trailMats[i] = Renderer::Material(Colors::MISSILE_TRAIL,
                                            MISSILE_TRAIL_BASE_ALPHA);
        m_trailMats[i].shadingMode = Renderer::ShadingMode::UNLIT;
        m_trailMats[i].emissive = true;

        t.obj = Primitives::createCube(MISSILE_TRAIL_PUFF_SIZE, MISSILE_TRAIL_PUFF_SIZE,
                                       MISSILE_TRAIL_PUFF_SIZE, &m_trailMats[i]);
        t.obj->cullingMode = Renderer::CullingMode::NO_CULLING;
        t.obj->ignoreZBuffer = true;
        m_scene.addObject(t.obj);
        stashSceneObject(t.obj);
        t.active = false;
    }
}

void ProjectileSystem::applyProjectileVisual(Projectile& p, ProjectileVisualKind kind) {
    Renderer::Material* mat = nullptr;

    if (kind == ProjectileVisualKind::PlayerMissile) {
        if (!p.missileObj) {
            p.missileObj = Primitives::createCube(MISSILE_VIS_W, MISSILE_VIS_H, MISSILE_VIS_LEN,
                                                    &m_missileMat);
            p.missileObj->cullingMode = Renderer::CullingMode::NO_CULLING;
            m_scene.addObject(p.missileObj);
            stashSceneObject(p.missileObj);
        }
        stashSceneObject(p.boltObj);
        stashSceneObject(p.bombObj);
        p.obj = p.missileObj;
        mat = &m_missileMat;
    } else if (kind == ProjectileVisualKind::Bolt) {
        if (!p.boltObj) {
            p.boltObj = Primitives::createSphere(6, 4, &m_enemyProjMat);
            m_scene.addObject(p.boltObj);
            stashSceneObject(p.boltObj);
        }
        stashSceneObject(p.missileObj);
        stashSceneObject(p.bombObj);
        p.obj = p.boltObj;
        mat = &m_enemyProjMat;
    } else if (kind == ProjectileVisualKind::Bomb) {
        if (!p.bombObj) {
            p.bombObj = Primitives::createCube(BOMB_VIS_SIZE, BOMB_VIS_SIZE, BOMB_VIS_SIZE,
                                               &m_bombMat);
            p.bombObj->cullingMode = Renderer::CullingMode::NO_CULLING;
            m_scene.addObject(p.bombObj);
            stashSceneObject(p.bombObj);
        }
        stashSceneObject(p.missileObj);
        stashSceneObject(p.boltObj);
        p.obj = p.bombObj;
        mat = &m_bombMat;
    }

    if (p.obj && mat) {
        for (auto& tri : p.obj->triangles) {
            tri.material = mat;
        }
        enableSceneObject(p.obj);
    }
    p.visualKind = kind;
}

void ProjectileSystem::updateMissileVisual(Projectile& p) {
    const float worldY = missileWorldY(p);
    showSceneObject(p.obj,
                    static_cast<int32_t>(p.x),
                    static_cast<int32_t>(worldY),
                    static_cast<int32_t>(p.z));

    const float horizSpeed = sqrtf(p.vx * p.vx + p.vz * p.vz);
    const float pitch = horizSpeed > 1.0f
        ? -atan2f(p.vy, horizSpeed) * 180.0f / static_cast<float>(M_PI)
        : 0.0f;
    const float yaw = atan2f(p.vx, p.vz) * 180.0f / static_cast<float>(M_PI);
    p.obj->setRotation((int16_t)pitch, (int16_t)yaw, 0);
}

void ProjectileSystem::spawnTrailPuff(float x, float y, float z) {
    for (size_t i = 0; i < m_trailPuffs.size(); ++i) {
        auto& t = m_trailPuffs[i];
        if (t.active) continue;

        t.active = true;
        t.life = MISSILE_TRAIL_LIFE;
        t.maxLife = MISSILE_TRAIL_LIFE;
        m_trailMats[i].alpha = MISSILE_TRAIL_BASE_ALPHA;
        showSceneObject(t.obj,
                        static_cast<int32_t>(x),
                        static_cast<int32_t>(y),
                        static_cast<int32_t>(z));
        return;
    }
}

void ProjectileSystem::firePlayerAtTarget(float x, float y, float z, float targetX, float targetZ,
                                          float targetAimY) {
    Projectile* slot = nullptr;
    for (auto& p : m_projectiles) {
        if (!p.active) {
            slot = &p;
            break;
        }
    }
    if (!slot) return;

    applyProjectileVisual(*slot, ProjectileVisualKind::PlayerMissile);

    float dx = targetX - x;
    float dz = targetZ - z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist < 1.0f) dist = 1.0f;

    const float angleToTarget = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
    const float side = Rng::coinFlip() ? 1.0f : -1.0f;
    const float launchAngle =
        (angleToTarget + side * MISSILE_SWOOP_DEG) * static_cast<float>(M_PI) / 180.0f;

    slot->x = x;
    slot->z = z;
    slot->prevX = slot->x;
    slot->prevZ = slot->z;
    slot->vx = sinf(launchAngle) * MISSILE_LAUNCH_SPEED;
    slot->vz = cosf(launchAngle) * MISSILE_LAUNCH_SPEED;
    const float launchHover = Terrain::hoverHeight(slot->x, slot->z);
    slot->y = y - launchHover;
    if (slot->y < 2.0f) slot->y = 2.0f;
    slot->launchWorldY = y;
    slot->vy = 0.0f;
    slot->targetX = targetX;
    slot->targetZ = targetZ;
    slot->targetAimY = targetAimY;
    slot->targetHoverY = Terrain::hoverHeight(targetX, targetZ);
    slot->targetYOffset = targetAimY - slot->targetHoverY;
    slot->launchDist = dist;
    slot->arcPathLen = dist * 1.45f;
    slot->pathTraveled = 0.0f;
    slot->peakArcY = MISSILE_PEAK_ARC_Y;
    slot->life = PROJECTILE_LIFE;
    slot->trailTimer = 0;
    slot->active = true;
    slot->isPlayerProjectile = true;
    slot->isHomingMissile = true;
    updateMissileVisual(*slot);
}

void ProjectileSystem::firePlayerStraight(float x, float y, float z, float targetX, float targetZ,
                                          float targetAimY) {
    Projectile* slot = nullptr;
    for (auto& p : m_projectiles) {
        if (!p.active) {
            slot = &p;
            break;
        }
    }
    if (!slot) return;

    applyProjectileVisual(*slot, ProjectileVisualKind::Bolt);

    float dx = targetX - x;
    float dz = targetZ - z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist < 1.0f) dist = 1.0f;

    const float speed = PROJECTILE_SPEED * 0.85f;
    slot->x = x;
    slot->z = z;
    slot->prevX = x;
    slot->prevZ = z;
    slot->vx = (dx / dist) * speed;
    slot->vz = (dz / dist) * speed;
    const float launchHover = Terrain::hoverHeight(x, z);
    slot->y = y - launchHover;
    if (slot->y < 0.0f) slot->y = 0.0f;
    slot->vy = 0;
    slot->life = PROJECTILE_LIFE * 0.6f;
    slot->trailTimer = 0;
    slot->active = true;
    slot->isPlayerProjectile = true;
    slot->isHomingMissile = false;
    slot->obj->setPosition((int16_t)slot->x,
                           (int16_t)(launchHover + slot->y),
                           (int16_t)slot->z);
}

void ProjectileSystem::fireEnemyAtTarget(float x, float z, float targetX, float targetZ) {
    Projectile* slot = nullptr;
    for (auto& p : m_projectiles) {
        if (!p.active) {
            slot = &p;
            break;
        }
    }
    if (!slot) return;

    applyProjectileVisual(*slot, ProjectileVisualKind::Bolt);

    float dx = targetX - x;
    float dz = targetZ - z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist < 1.0f) dist = 1.0f;

    const float speed = PROJECTILE_SPEED * 0.65f;
    slot->x = x;
    slot->z = z;
    slot->prevX = x;
    slot->prevZ = z;
    slot->vx = (dx / dist) * speed;
    slot->vz = (dz / dist) * speed;
    slot->y = 0;
    slot->vy = 0;
    slot->life = PROJECTILE_LIFE;
    slot->trailTimer = 0;
    slot->active = true;
    slot->isPlayerProjectile = false;
    slot->isHomingMissile = false;
    slot->obj->setPosition((int16_t)slot->x,
                           (int16_t)Terrain::hoverHeight(slot->x, slot->z),
                           (int16_t)slot->z);
}

void ProjectileSystem::fireEnemyBomb(float x, float y, float z, float targetX, float targetZ) {
    Projectile* slot = nullptr;
    for (auto& p : m_projectiles) {
        if (!p.active) {
            slot = &p;
            break;
        }
    }
    if (!slot) return;

    applyProjectileVisual(*slot, ProjectileVisualKind::Bomb);

    float dx = targetX - x;
    float dz = targetZ - z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist < 1.0f) dist = 1.0f;

    slot->x = x;
    slot->z = z;
    slot->prevX = x;
    slot->prevZ = z;
    slot->y = y;
    slot->vy = -60.0f;
    slot->vx = (dx / dist) * 45.0f;
    slot->vz = (dz / dist) * 45.0f;
    slot->life = 4.0f;
    slot->trailTimer = 0;
    slot->active = true;
    slot->isPlayerProjectile = false;
    slot->isHomingMissile = false;
    slot->isFallingBomb = true;
    slot->obj->setPosition((int16_t)slot->x, (int16_t)slot->y, (int16_t)slot->z);
}

void ProjectileSystem::fireEnemyHomingAtTarget(float x, float y, float z, float targetX,
                                               float targetZ, float targetAimY,
                                               float damageScale) {
    Projectile* slot = nullptr;
    for (auto& p : m_projectiles) {
        if (!p.active) {
            slot = &p;
            break;
        }
    }
    if (!slot) return;

    applyProjectileVisual(*slot, ProjectileVisualKind::PlayerMissile);

    float dx = targetX - x;
    float dz = targetZ - z;
    float dist = sqrtf(dx * dx + dz * dz);
    if (dist < 1.0f) dist = 1.0f;

    const float angleToTarget = atan2f(dx, dz) * 180.0f / static_cast<float>(M_PI);
    const float side = Rng::coinFlip() ? 1.0f : -1.0f;
    const float launchAngle =
        (angleToTarget + side * MISSILE_SWOOP_DEG) * static_cast<float>(M_PI) / 180.0f;

    slot->x = x;
    slot->z = z;
    slot->prevX = slot->x;
    slot->prevZ = slot->z;
    slot->vx = sinf(launchAngle) * MISSILE_LAUNCH_SPEED;
    slot->vz = cosf(launchAngle) * MISSILE_LAUNCH_SPEED;
    const float launchHover = Terrain::hoverHeight(slot->x, slot->z);
    slot->y = y - launchHover;
    if (slot->y < 2.0f) slot->y = 2.0f;
    slot->launchWorldY = y;
    slot->vy = 0.0f;
    slot->targetX = targetX;
    slot->targetZ = targetZ;
    slot->targetAimY = targetAimY;
    slot->targetHoverY = Terrain::hoverHeight(targetX, targetZ);
    slot->targetYOffset = targetAimY - slot->targetHoverY;
    slot->launchDist = dist;
    slot->arcPathLen = dist * 1.45f;
    slot->pathTraveled = 0.0f;
    slot->peakArcY = MISSILE_PEAK_ARC_Y;
    slot->life = PROJECTILE_LIFE;
    slot->trailTimer = 0;
    slot->active = true;
    slot->isPlayerProjectile = false;
    slot->isHomingMissile = true;
    slot->damageScale = damageScale;
    updateMissileVisual(*slot);
}

void ProjectileSystem::detonateMissile(Projectile& p) {
    const float worldY = missileWorldY(p);
    spawnTrailPuff(p.x, worldY, p.z);
    spawnTrailPuff(p.x + 4.0f, worldY + 2.0f, p.z - 2.0f);
    destroyProjectile(p);
}

void ProjectileSystem::destroyProjectile(Projectile& p) {
    p.active = false;
    p.isHomingMissile = false;
    p.isFallingBomb = false;
    p.damageScale = 1.0f;
    stashSceneObject(p.missileObj);
    stashSceneObject(p.boltObj);
    stashSceneObject(p.bombObj);
    p.obj = nullptr;
    p.visualKind = ProjectileVisualKind::None;
}

float ProjectileSystem::missileWorldY(const Projectile& p) const {
    return Terrain::hoverHeight(p.x, p.z) + p.y;
}

float ProjectileSystem::computeMissileDesiredYOffset(const Projectile& p,
                                                     float progress) const {
    if (progress < 0.0f) progress = 0.0f;
    else if (progress > 1.0f) progress = 1.0f;

    // Single smooth bell: 0 at launch and impact, peak near mid-flight.
    float arcFactor = sinf(progress * static_cast<float>(M_PI));
    if (progress > 0.72f) {
        const float t = (progress - 0.72f) / 0.28f;
        const float clamped = (t > 1.0f) ? 1.0f : t;
        arcFactor *= (1.0f - clamped);
    }

    const float baseWorldY =
        p.launchWorldY + (p.targetAimY - p.launchWorldY) * progress;
    const float desiredWorldY = baseWorldY + p.peakArcY * arcFactor;

    const float localHover = Terrain::hoverHeight(p.x, p.z);
    float refHover = localHover;
    if (progress > 0.78f) {
        const float t = (progress - 0.78f) / 0.22f;
        const float blend = (t > 1.0f) ? 1.0f : t;
        refHover = localHover + (p.targetHoverY - localHover) * blend;
    }

    return desiredWorldY - refHover;
}

void ProjectileSystem::updateHomingMissile(Projectile& p, float deltaTime) {
    const float dx = p.targetX - p.x;
    const float dz = p.targetZ - p.z;
    const float horizDist = sqrtf(dx * dx + dz * dz);
    const float desiredAngle = atan2f(dx, dz);
    const float currentAngle = atan2f(p.vx, p.vz);
    float angleDiff = wrapAngleDeg(
        (desiredAngle - currentAngle) * 180.0f / static_cast<float>(M_PI));

    const float maxTurn = MISSILE_TURN_RATE * deltaTime;
    if (angleDiff > maxTurn) angleDiff = maxTurn;
    else if (angleDiff < -maxTurn) angleDiff = -maxTurn;

    const float newAngle =
        currentAngle + angleDiff * static_cast<float>(M_PI) / 180.0f;
    float speed = sqrtf(p.vx * p.vx + p.vz * p.vz);
    speed += (MISSILE_CRUISE_SPEED - speed) * 2.5f * deltaTime;
    if (speed > MISSILE_CRUISE_SPEED) speed = MISSILE_CRUISE_SPEED;

    p.vx = sinf(newAngle) * speed;
    p.vz = cosf(newAngle) * speed;

    p.pathTraveled += speed * deltaTime;

    p.prevX = p.x;
    p.prevZ = p.z;
    p.x += p.vx * deltaTime;
    p.z += p.vz * deltaTime;

    const float horizDistAfter =
        sqrtf((p.targetX - p.x) * (p.targetX - p.x) + (p.targetZ - p.z) * (p.targetZ - p.z));

    float pathProgress = p.pathTraveled / p.arcPathLen;
    if (pathProgress > 1.0f) pathProgress = 1.0f;

    float distProgress = 1.0f - (horizDistAfter / p.launchDist);
    if (distProgress < 0.0f) distProgress = 0.0f;
    else if (distProgress > 1.0f) distProgress = 1.0f;

    const float progress = (pathProgress > distProgress) ? pathProgress : distProgress;
    const float desiredYOffset = computeMissileDesiredYOffset(p, progress);

    const float track = 14.0f * deltaTime;
    const float t = (track > 1.0f) ? 1.0f : track;
    p.y += (desiredYOffset - p.y) * t;
    p.vy = (desiredYOffset - p.y) / (deltaTime > 0.0001f ? deltaTime : 0.0001f);

    p.trailTimer -= deltaTime;
    if (p.trailTimer <= 0.0f) {
        const float worldY = missileWorldY(p);
        spawnTrailPuff(p.x, worldY, p.z);
        p.trailTimer = MISSILE_TRAIL_INTERVAL;
    }

    updateMissileVisual(p);
}

void ProjectileSystem::updateStraightProjectile(Projectile& p, float deltaTime) {
    p.prevX = p.x;
    p.prevZ = p.z;
    p.x += p.vx * deltaTime;
    p.z += p.vz * deltaTime;
    showSceneObject(p.obj,
                    static_cast<int32_t>(p.x),
                    static_cast<int32_t>(Terrain::hoverHeight(p.x, p.z)),
                    static_cast<int32_t>(p.z));
}

void ProjectileSystem::updateFallingBomb(Projectile& p, float deltaTime) {
    p.prevX = p.x;
    p.prevZ = p.z;
    p.vy -= BOMB_GRAVITY * deltaTime;
    p.x += p.vx * deltaTime;
    p.z += p.vz * deltaTime;
    p.y += p.vy * deltaTime;

    const float groundY = Terrain::hoverHeight(p.x, p.z);
    if (p.y <= groundY + 3.0f) {
        p.y = groundY + 3.0f;
        destroyProjectile(p);
        return;
    }

    showSceneObject(p.obj,
                    static_cast<int32_t>(p.x),
                    static_cast<int32_t>(p.y),
                    static_cast<int32_t>(p.z));
}

void ProjectileSystem::update(float deltaTime) {
    for (size_t i = 0; i < m_trailPuffs.size(); ++i) {
        auto& t = m_trailPuffs[i];
        if (!t.active) continue;

        t.life -= deltaTime;
        const float lifeRatio = (t.maxLife > 0.0f) ? (t.life / t.maxLife) : 0.0f;
        const int alpha = (int)(MISSILE_TRAIL_BASE_ALPHA * lifeRatio);
        m_trailMats[i].alpha = (uint8_t)(alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha));

        if (t.life <= 0.0f || m_trailMats[i].alpha < 8) {
            t.active = false;
            stashSceneObject(t.obj);
        }
    }

    for (auto& p : m_projectiles) {
        if (!p.active) continue;

        p.life -= deltaTime;
        if (p.life <= 0) {
            destroyProjectile(p);
            continue;
        }

        if (p.isHomingMissile) {
            updateHomingMissile(p, deltaTime);
        } else if (p.isFallingBomb) {
            updateFallingBomb(p, deltaTime);
        } else {
            updateStraightProjectile(p, deltaTime);
        }
    }
}

int ProjectileSystem::checkPlayerHit(float playerX, float playerZ, float playerAimY,
                                     float playerWidth,
                                     float* outHitX, float* outHitZ, float* outHitY) {
    const float hitDist = playerWidth / 2 + 18;
    const auto scaledDamage = [this](int baseDamage, float perProjectileScale) {
        return static_cast<int>(lroundf(
            static_cast<float>(baseDamage) * m_enemyDamageScale * perProjectileScale));
    };

    auto recordHit = [&](float x, float y, float z) {
        if (outHitX) {
            *outHitX = x;
        }
        if (outHitY) {
            *outHitY = y;
        }
        if (outHitZ) {
            *outHitZ = z;
        }
    };

    for (auto& p : m_projectiles) {
        if (!p.active || p.isPlayerProjectile) continue;

        if (p.isHomingMissile) {
            const float dx = p.x - playerX;
            const float dz = p.z - playerZ;
            const float dist = sqrtf(dx * dx + dz * dz);

            const float segDist = distPointToSegment(
                playerX, playerZ, p.prevX, p.prevZ, p.x, p.z);
            const float closest = (dist < segDist) ? dist : segDist;

            const float missileWorldY = Terrain::hoverHeight(p.x, p.z) + p.y;
            const float yDist = fabsf(missileWorldY - playerAimY);

            if (closest < ENEMY_HIT_RADIUS && yDist < ENEMY_HIT_Y_TOLERANCE) {
                recordHit(p.x, missileWorldY, p.z);
                detonateMissile(p);
                return scaledDamage(PLAYER_DAMAGE_ENEMY_MISSILE, p.damageScale);
            }
            continue;
        }

        if (p.isFallingBomb) {
            const float groundY = Terrain::hoverHeight(p.x, p.z);
            if (p.y > groundY + 18.0f) continue;

            const float dx = p.x - playerX;
            const float dz = p.z - playerZ;
            const float dist = sqrtf(dx * dx + dz * dz);
            const float segDist = distPointToSegment(
                playerX, playerZ, p.prevX, p.prevZ, p.x, p.z);
            const float closest = (dist < segDist) ? dist : segDist;

            if (closest < BOMB_SPLASH_RADIUS) {
                recordHit(p.x, p.y, p.z);
                destroyProjectile(p);
                return scaledDamage(PLAYER_DAMAGE_BOMB, p.damageScale);
            }
            continue;
        }

        const float dx = p.x - playerX;
        const float dz = p.z - playerZ;
        const float dist = sqrtf(dx * dx + dz * dz);

        if (dist < hitDist) {
            recordHit(p.x, Terrain::hoverHeight(p.x, p.z), p.z);
            destroyProjectile(p);
            return scaledDamage(PLAYER_DAMAGE_TANK_BOLT, p.damageScale);
        }
    }
    return 0;
}

bool ProjectileSystem::checkEnemyHit(float enemyX, float enemyZ, float enemyMinY,
                                     float enemyMaxY, float enemyWidth) {
    (void)enemyWidth;

    for (auto& p : m_projectiles) {
        if (!p.active || !p.isPlayerProjectile) continue;

        const float dx = p.x - enemyX;
        const float dz = p.z - enemyZ;
        const float dist = sqrtf(dx * dx + dz * dz);

        const float segDist = distPointToSegment(
            enemyX, enemyZ, p.prevX, p.prevZ, p.x, p.z);
        const float closest = (dist < segDist) ? dist : segDist;

        const float missileWorldY = Terrain::hoverHeight(p.x, p.z) + p.y;
        const bool yHit =
            (missileWorldY >= enemyMinY && missileWorldY <= enemyMaxY);

        if (closest < ENEMY_HIT_RADIUS && yHit) {
            destroyProjectile(p);
            return true;
        }
    }
    return false;
}

bool ProjectileSystem::checkMissileTargetImpact(float* outX, float* outZ, float* outY) {
    bool hit = false;

    for (auto& p : m_projectiles) {
        if (!p.active || !p.isHomingMissile || !p.isPlayerProjectile) continue;

        const float dx = p.targetX - p.x;
        const float dz = p.targetZ - p.z;
        const float horizDist = sqrtf(dx * dx + dz * dz);

        const float segDist = distPointToSegment(
            p.targetX, p.targetZ, p.prevX, p.prevZ, p.x, p.z);
        const float closest = (horizDist < segDist) ? horizDist : segDist;

        const float missileWorldY = Terrain::hoverHeight(p.x, p.z) + p.y;
        const float yDist = fabsf(missileWorldY - p.targetAimY);

        if (closest < ENEMY_HIT_RADIUS && yDist < ENEMY_HIT_Y_TOLERANCE) {
            if (outX) *outX = p.targetX;
            if (outZ) *outZ = p.targetZ;
            if (outY) *outY = p.targetAimY;
            detonateMissile(p);
            hit = true;
        }
    }

    return hit;
}

void ProjectileSystem::reset() {
    for (auto& p : m_projectiles) {
        destroyProjectile(p);
    }
    for (auto& t : m_trailPuffs) {
        t.active = false;
        stashSceneObject(t.obj);
    }
}

void ProjectileSystem::setEnemyDamageScale(float scale) {
    m_enemyDamageScale = scale;
}

} // namespace Game
