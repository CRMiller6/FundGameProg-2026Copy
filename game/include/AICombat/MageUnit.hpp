#pragma once

#include <Canis/Entity.hpp>

#include <string>

namespace AICombat
{
    class MageUnit : public Canis::ScriptableEntity
    {
    public:
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

        explicit MageUnit(Canis::Entity& _entity) : Canis::ScriptableEntity(_entity) {}

        void Create() override;
        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;

    private:
        float m_fireCooldown = 0.0f;
        Canis::Entity* m_target = nullptr;

        Canis::Entity* FindTarget() const;
        Canis::Vector3 GetMuzzlePosition(const Canis::Transform& _transform) const;
        float RotateTowards(Canis::Transform& _transform, const Canis::Vector3& _direction, float _dt) const;
        void Fire(const Canis::Vector3& _position, const Canis::Vector3& _direction);
    };

    void RegisterMageUnitScript(Canis::App& _app);
    void UnRegisterMageUnitScript(Canis::App& _app);
}
