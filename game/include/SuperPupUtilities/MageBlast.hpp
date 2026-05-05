#pragma once

#include <Canis/Entity.hpp>

#include <string>
#include <vector>

namespace SuperPupUtilities
{
    class MageBlast : public Canis::ScriptableEntity
    {
    public:
        static constexpr const char* ScriptName = "SuperPupUtilities::MageBlast";

        int damage = 1;
        float speed = 20.0f;
        float lifeTime = 10.0f;
        float gravity = 0.0f;
        bool destroyOnImpact = true;
        bool destroyEntityWhenDone = false;
        bool autoLaunch = false;
        Canis::Mask collisionMask = Canis::Rigidbody::DefaultMask;
        float hitImpulse = 0.0f;
        std::vector<std::string> targetTags = {};

        explicit MageBlast(Canis::Entity& _entity) : Canis::ScriptableEntity(_entity) {}

        void Create() override;
        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;

        void Launch();
        void Launch(const Canis::Vector3& _position, const Canis::Vector3& _rotation);
        void DestroyMageBlast();
        bool IsLaunched() const;

    private:
        bool m_launched = false;
        float m_timeRemaining = 0.0f;
        Canis::Vector3 m_lastPosition = Canis::Vector3(0.0f);

        void ResetLifetime();
        void Move(float _dt);
        void CollisionCheck(const Canis::Vector3& _start, const Canis::Vector3& _end);
        bool IsValidTarget(const Canis::Entity& _entity) const;
    };

    void RegisterMageBlastScript(Canis::App& _app);
    void UnRegisterMageBlastScript(Canis::App& _app);
}
