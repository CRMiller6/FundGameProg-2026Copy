#include <AICombat/MageUnit.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>
#include <SuperPupUtilities/Bullet.hpp>
#include <SuperPupUtilities/SimpleObjectPool.hpp>

#include <algorithm>
#include <cmath>

namespace AICombat
{

    Canis::ScriptConf MageUnitConf = {};

    MageUnit::MageUnit(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity) {}

    void RegisterMageUnitScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, laserPrefab);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, targetTag);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, poolCode);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, fireInterval);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, turnSpeedDegrees);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, fireAngleThresholdDegrees);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, muzzleOffset);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, projectileSpeed);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, projectileLifeTime);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, projectileHitImpulse);

        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, maxHealth);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, logStateChanges);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, deathEffectPrefab);

        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, hitSfxPath1);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, hitSfxPath2);
        REGISTER_PROPERTY(MageUnitConf, AICombat::MageUnit, hitSfxVolume);

        DEFAULT_CONFIG_AND_REQUIRED(
            MageUnitConf,
            AICombat::MageUnit,
            Canis::Transform);

        MageUnitConf.DEFAULT_DRAW_INSPECTOR(AICombat::MageUnit);

        _app.RegisterScript(MageUnitConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(MageUnitConf, MageUnit)

    void MageUnit::Create()
    {
        entity.GetComponent<Canis::Transform>();

        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }
    }

    void MageUnit::Ready()
    {
        m_currentHealth = std::max(maxHealth, 1);

        m_target = FindTarget();
        m_fireCooldown = fireInterval;

        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }

        m_useFirstHitSfx = true;
    }

    void MageUnit::Destroy() {}

    void MageUnit::Update(float _dt)
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

    Canis::Entity* MageUnit::FindTarget() const
    {
        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
        {
            if (candidate != nullptr && candidate->active)
                return candidate;
        }

        return nullptr;
    }

    Canis::Vector3 MageUnit::GetMuzzlePosition(const Canis::Transform& _transform) const
    {
        return _transform.GetGlobalPosition()
            + (_transform.GetRight() * muzzleOffset.x)
            + (_transform.GetUp() * muzzleOffset.y)
            + (_transform.GetForward() * muzzleOffset.z);
    }

    float MageUnit::RotateTowards(Canis::Transform& _transform, const Canis::Vector3& _direction, float _dt) const
    {
        const Canis::Vector3 flatDirection = glm::normalize(Canis::Vector3(_direction.x, 0.0f, _direction.z));
        const float targetYaw = std::atan2(-flatDirection.x, -flatDirection.z);
        const float yawError = std::remainder(targetYaw - _transform.rotation.y, TAU);
        const float maxStep = turnSpeedDegrees * Canis::DEG2RAD * _dt;
        const float appliedStep = std::clamp(yawError, -maxStep, maxStep);

        _transform.rotation.y += appliedStep;
        return std::remainder(targetYaw - _transform.rotation.y, TAU);
    }

    void MageUnit::Fire(const Canis::Vector3& _position, const Canis::Vector3& _direction)
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

        if (pool == nullptr) {
            Debug::Log("Pool is null");
            return; }

        Canis::Entity* projectile = pool->Spawn(poolCode, _position, rotation);

        if (projectile == nullptr) {
            Debug::Log("projectile is null");
            return; }

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

    int MageUnit::GetCurrentHealth() const
    {
        return m_currentHealth;
    }

    void MageUnit::Healing(int newHealth){
        m_currentHealth = std::clamp(newHealth, 0, maxHealth);
    }

    void MageUnit::TakeDamage(int _damage)
    {
        Debug::Log("MageUnit Take Damage");
        if (!IsAlive())
            return;

        const int damageToApply = std::max(_damage, 0);
        if (damageToApply <= 0)
            return;

        m_currentHealth = std::max(0, m_currentHealth - damageToApply);
        PlayHitSfx();

        if (m_hasBaseColor && entity.HasComponent<Canis::Material>())
        {
            Canis::Material& material = entity.GetComponent<Canis::Material>();
            const float healthRatio = (maxHealth > 0)
                ? (static_cast<float>(m_currentHealth) / static_cast<float>(maxHealth))
                : 0.0f;

            material.color = Canis::Vector4(
                m_baseColor.x * (0.5f + (0.5f * healthRatio)),
                m_baseColor.y * (0.5f + (0.5f * healthRatio)),
                m_baseColor.z * (0.5f + (0.5f * healthRatio)),
                m_baseColor.w);
        }

        if (m_currentHealth > 0)
            return;

        if (logStateChanges)
            Canis::Debug::Log("%s was defeated.", entity.name.c_str());

        SpawnDeathEffect();
        entity.Destroy();
    }

    void MageUnit::SpawnDeathEffect()
    {
        Debug::Log("MU SpawnDeathEffect");
        if (deathEffectPrefab.Empty() || !entity.HasComponent<Canis::Transform>())
            return;

        const Canis::Transform& sourceTransform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 spawnPosition = sourceTransform.GetGlobalPosition();
        const Canis::Vector3 spawnRotation = sourceTransform.GetGlobalRotation();

        for (Canis::Entity* spawnedEntity : entity.scene.Instantiate(deathEffectPrefab))
        {
            if (spawnedEntity == nullptr || !spawnedEntity->HasComponent<Canis::Transform>())
                continue;

            Canis::Transform& spawnedTransform = spawnedEntity->GetComponent<Canis::Transform>();
            spawnedTransform.position = spawnPosition;
            spawnedTransform.rotation = spawnRotation;
        }
    }

    bool MageUnit::IsAlive() const
    {
        return m_currentHealth > 0;
    }

    void MageUnit::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }
}
