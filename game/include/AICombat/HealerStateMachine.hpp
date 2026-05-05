#pragma once
#include <Canis/Entity.hpp>
#include <SuperPupUtilities/StateMachine.hpp>
#include <string>

namespace AICombat {
    class HealerStateMachine;

    class HealerIdleState : public SuperPupUtilities::State {
    public:
        static constexpr const char* Name = "HealerIdleState";
        explicit HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Update(float _dt) override;
    };

    class HealState : public SuperPupUtilities::State {
    public:
        static constexpr const char* Name = "HealState";
        float moveSpeed = 4.0f;
        float healRange = 3.0f;
        explicit HealState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    private:
        float m_healTimer = 0.0f;
    };

    class HealerStateMachine : public SuperPupUtilities::StateMachine {
    public:
        static constexpr const char* ScriptName = "AICombat::HealerStateMachine";
        
        std::string teamTag = ""; 
        float detectionRange = 20.0f;
        int healAmount = 1;
        float healInterval = 5.0f;
        bool logStateChanges = true;
        Canis::Entity* currentTarget = nullptr;

        explicit HealerStateMachine(Canis::Entity& _entity);

        HealerIdleState idleState;
        HealState healState;

        void Create() override;
        void Ready() override;
        void Update(float _dt) override;

        Canis::Entity* FindWoundedTeammate() const;
        void MoveTowards(const Canis::Entity& _target, float _speed, float _dt);
        void ChangeState(const std::string& _stateName);

    private:
        float m_stateTime = 0.0f;
    };

    void RegisterHealerStateMachineScript(Canis::App& _app);
    void UnRegisterHealerStateMachineScript(Canis::App& _app);
}
