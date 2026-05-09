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
        static constexpr const char* ScriptName = "AICombat::HealerStateMachine";
        
        std::string teamTag = ""; 
        float detectionRange = 20.0f;
        int healAmount = 1;
        int maxHealth;
        float healInterval = 5.0f;
        bool logStateChanges = true;
        Canis::Entity* currentTarget = nullptr;

        explicit HealerStateMachine(Canis::Entity& _entity);

        HealerIdleState idleState;
        HealState healState;
        HealerChaseState chaseState;

        void Create() override;
        void Ready() override;
        void Update(float _dt) override;

        Canis::Entity* FindWoundedTeammate() const;
        void MoveTowards(const Canis::Entity& _target, float _speed, float _dt);
        void ChangeState(const std::string& _stateName);
        void TakeDamage(int _damage);

        bool IsAlive() const;

        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "assets/audio/sfx/hit_2.ogg" };
        float hitSfxVolume = 1.0f;

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
