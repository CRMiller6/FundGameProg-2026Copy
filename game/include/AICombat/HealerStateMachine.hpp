#pragma once
#include <Canis/Entity.hpp>
#include <SuperPupUtilities/StateMachine.hpp>
#include <string>

namespace AICombat {
    

    class HealerIdleState : public SuperPupUtilities::State {
    public:
        static constexpr const char* Name = "HealerIdleState";
        explicit HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Update(float _dt) override {}
    };

    class HealState : public SuperPupUtilities::State {
    public:
        static constexpr const char* Name = "HealState";
        float moveSpeed = 4.0f;
        float healRange = 3.0f;
        explicit HealState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override {}
        void Update(float _dt) override {}
    private:
        float m_healTimer = 0.0f;
    };

    class HealerChaseState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "ChaseState";
        float moveSpeed = 4.0f;

        explicit HealerChaseState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class HealerStateMachine : public SuperPupUtilities::StateMachine {
    public:
        float healCooldown = 0.0f;
        float healDelay = 2.0f;

        static constexpr const char* ScriptName = "AICombat::HealerStateMachine";
        
        std::string teamTag = ""; 
        float detectionRange = 20.0f;
        int healAmount = 2;
        int maxHealth = 30;
        float healInterval = 5.0f;
        bool logStateChanges = true;
        Canis::Entity* currentTarget = nullptr;

        float moveSpeed = 5.0f;
        float m_healAccumulator = 0.0f;

        explicit HealerStateMachine(Canis::Entity& _entity);

        HealerIdleState idleState;
        HealState healState;
        HealerChaseState chaseState;

        void Create() override;
        void Ready() override;
        void Update(float _dt) override;

        Canis::Entity* FindWoundedTeammate() const;
        void MoveTowards(Canis::Vector3 _targetpos, float _speed, float _dt);
        void ChangeState(const std::string& _stateName);
        void TakeDamage(int _damage);

        bool IsAlive() const;

        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "assets/audio/sfx/hit_2.ogg" };
        float hitSfxVolume = 1.0f;

        Canis::SceneAssetHandle deathEffectPrefab = { .path = "assets/prefabs/brawler_death_particles.scene" };

    private:
        void PlayHitSfx();
        void SpawnDeathEffect();

        float m_stateTime = 0.0f;
        int m_currentHealth = 0;
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;
        bool m_useFirstHitSfx = true;
    };

    void RegisterHealerStateMachineScript(Canis::App& _app);
    void UnRegisterHealerStateMachineScript(Canis::App& _app);
}
