#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/StateMachine.hpp>

#include <string>

namespace AICombat
{
    class MageUnit : public SuperPupUtilities::StateMachine
    {
    public:
        int maxHealth = 40;
        void Heal(int _amount) {
            if (!IsAlive()) return;
            m_currentHealth = std::min(maxHealth, m_currentHealth + _amount);
        }

        static constexpr const char* ScriptName = "AICombat::MageUnit";

        Canis::SceneAssetHandle laserPrefab = {};
        std::string targetTag = "Player";
        std::string poolCode = "laser_bullet";
        float fireInterval = 1.75f;
        float turnSpeedDegrees = 120.0f;
        float fireAngleThresholdDegrees = 8.0f;
        Canis::Vector3 muzzleOffset = Canis::Vector3(0.0f, 0.15f, 1.0f);
        float projectileSpeed = 14.0f;
        float projectileLifeTime = 4.0f;
        float projectileHitImpulse = 6.0f;
        Canis::SceneAssetHandle deathEffectPrefab = { .path = "assets/prefabs/brawler_death_particles.scene" };

        void Create() override;
        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;
        void TakeDamage(int _damage);
        bool IsAlive() const;
        void Healing(int newHealth);
        int GetCurrentHealth() const;

        bool logStateChanges = true;

        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "assets/audio/sfx/hit_2.ogg" };
        float hitSfxVolume = 1.0f;

    private:
        int m_currentHealth = 0;
        float m_fireCooldown = 0.0f;
        Canis::Entity* m_target = nullptr;

        Canis::Entity* FindTarget() const;
        Canis::Vector3 GetMuzzlePosition(const Canis::Transform& _transform) const;
        float RotateTowards(Canis::Transform& _transform, const Canis::Vector3& _direction, float _dt) const;
        void Fire(const Canis::Vector3& _position, const Canis::Vector3& _direction);

        void PlayHitSfx();
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;

        void SpawnDeathEffect();
        bool m_useFirstHitSfx = true;
    };

    void RegisterMageUnitScript(Canis::App& _app);
    void UnRegisterMageUnitScript(Canis::App& _app);
}
