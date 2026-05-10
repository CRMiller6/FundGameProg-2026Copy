#include <RollABall/LaserTagTurret.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>
#include <SuperPupUtilities/Bullet.hpp>
#include <SuperPupUtilities/SimpleObjectPool.hpp>

#include <algorithm>
#include <cmath>

namespace RollABall
{

    Canis::ScriptConf laserTagTurretConf = {};

    void RegisterLaserTagTurretScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, laserPrefab);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, targetTag);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, poolCode);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, fireInterval);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, turnSpeedDegrees);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, fireAngleThresholdDegrees);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, muzzleOffset);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, projectileSpeed);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, projectileLifeTime);
        REGISTER_PROPERTY(laserTagTurretConf, RollABall::LaserTagTurret, projectileHitImpulse);

        DEFAULT_CONFIG_AND_REQUIRED(laserTagTurretConf, RollABall::LaserTagTurret, Canis::Transform);

        laserTagTurretConf.DEFAULT_DRAW_INSPECTOR(RollABall::LaserTagTurret);

        _app.RegisterScript(laserTagTurretConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(laserTagTurretConf, LaserTagTurret)

    void LaserTagTurret::Create()
    {
        entity.GetComponent<Canis::Transform>();
    }

    void LaserTagTurret::Ready()
    {
        m_target = FindTarget();
        m_fireCooldown = fireInterval;
    }

    void LaserTagTurret::Destroy() {}

    void LaserTagTurret::Update(float _dt)
    {
        if (!entity.HasComponent<Canis::Transform>())
            return;

        if (m_target == nullptr || !m_target->active || m_target->tag != targetTag)
            m_target = FindTarget();

        if (m_target == nullptr || !m_target->HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 targetPosition = m_target->GetComponent<Canis::Transform>().GetGlobalPosition();
        Canis::Vector3 toTarget = targetPosition - transform.GetGlobalPosition();
        Canis::Vector3 flatToTarget = toTarget;
        flatToTarget.y = 0.0f;

        if (glm::length(flatToTarget) <= 0.001f)
            return;

        const float angleError = RotateTowards(transform, flatToTarget, _dt);
        if (m_fireCooldown > 0.0f)
            m_fireCooldown -= _dt;

        if (m_fireCooldown > 0.0f)
            return;

        const float fireAngleThreshold = fireAngleThresholdDegrees * Canis::DEG2RAD;
        if (std::abs(angleError) > fireAngleThreshold)
            return;

        const Canis::Vector3 muzzlePosition = GetMuzzlePosition(transform);
        Fire(muzzlePosition, targetPosition - muzzlePosition);
        m_fireCooldown = fireInterval;
    }

    Canis::Entity* LaserTagTurret::FindTarget() const
    {
        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
        {
            if (candidate != nullptr && candidate->active)
                return candidate;
        }

        return nullptr;
    }

    Canis::Vector3 LaserTagTurret::GetMuzzlePosition(const Canis::Transform& _transform) const
    {
        return _transform.GetGlobalPosition()
            + (_transform.GetRight() * muzzleOffset.x)
            + (_transform.GetUp() * muzzleOffset.y)
            + (_transform.GetForward() * muzzleOffset.z);
    }

    float LaserTagTurret::RotateTowards(Canis::Transform& _transform, const Canis::Vector3& _direction, float _dt) const
    {
        const Canis::Vector3 flatDirection = glm::normalize(Canis::Vector3(_direction.x, 0.0f, _direction.z));
        const float targetYaw = std::atan2(-flatDirection.x, -flatDirection.z);
        const float yawError = std::remainder(targetYaw - _transform.rotation.y, TAU);
        const float maxStep = turnSpeedDegrees * Canis::DEG2RAD * _dt;
        const float appliedStep = std::clamp(yawError, -maxStep, maxStep);

        _transform.rotation.y += appliedStep;
        return std::remainder(targetYaw - _transform.rotation.y, TAU);
    }

    void LaserTagTurret::Fire(const Canis::Vector3& _position, const Canis::Vector3& _direction)
    {
        const float directionLength = glm::length(_direction);
        const Canis::Vector3 flatDirection = Canis::Vector3(_direction.x, 0.0f, _direction.z);
        if (directionLength <= 0.001f || glm::length(flatDirection) <= 0.001f)
            return;

        const Canis::Vector3 normalizedDirection = _direction / directionLength;
        const Canis::Vector3 normalizedFlatDirection = glm::normalize(flatDirection);
        const float pitch = std::asin(std::clamp(normalizedDirection.y, -1.0f, 1.0f));
        const float yaw = std::atan2(-normalizedFlatDirection.x, -normalizedFlatDirection.z);
        const Canis::Vector3 rotation = Canis::Vector3(pitch, yaw, 0.0f);

        auto* pool = SuperPupUtilities::SimpleObjectPool::Instance;

        if (pool == nullptr)
            return;

        Canis::Entity* projectile = pool->Spawn(poolCode, _position, rotation);

        if (projectile == nullptr)
            return;

        if (SuperPupUtilities::Bullet* bullet = projectile->GetScript<SuperPupUtilities::Bullet>())
        {
            bullet->speed = projectileSpeed*10.0f;
            bullet->lifeTime = projectileLifeTime;
            bullet->hitImpulse = projectileHitImpulse;
            bullet->collisionMask = Canis::Rigidbody::DefaultMask;
            if (m_target != nullptr && m_target->HasComponent<Canis::Rigidbody>())
                bullet->collisionMask = m_target->GetComponent<Canis::Rigidbody>().layer;
            bullet->Launch();
        }
    }
}
