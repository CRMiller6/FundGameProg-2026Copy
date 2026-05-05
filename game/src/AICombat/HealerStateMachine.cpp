#include <AICombat/HealerStateMachine.hpp>
#include <AICombat/BrawlerStateMachine.hpp>
#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>
#include <algorithm>

namespace AICombat {

namespace {
    ScriptConf healerStateMachineConf = {};
}

HealerIdleState::HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

HealState::HealState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

Canis::Entity* HealerStateMachine::FindWoundedTeammate() const {
    if (teamTag.empty()) return nullptr;
    
    Canis::Entity* targetToHeal = nullptr;
    int lowestHealthFound = 999999;
    
    Canis::Vector3 myPos = entity.GetComponent<Canis::Transform>().GetGlobalPosition();

    for (Canis::Entity* ally : entity.scene.GetEntitiesWithTag(teamTag)) {
        if (!ally || ally == &entity || !ally->active) continue;
        
        BrawlerStateMachine* brawler = ally->GetScript<BrawlerStateMachine>();
        
        if (brawler && brawler->IsAlive()) {
            int currentHP = brawler->GetCurrentHealth();
            
            if (currentHP < 40) { 
                float dist = glm::distance(myPos, ally->GetComponent<Canis::Transform>().GetGlobalPosition());
                
                if (dist <= detectionRange) {
                    if (currentHP < lowestHealthFound) {
                        lowestHealthFound = currentHP;
                        targetToHeal = ally;
                    }
                }
            }
        }
    }
    
    return targetToHeal;
} 

HealerStateMachine::HealerStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        idleState(*this),
        healState(*this) {}

void RegisterHealerStateMachineScript(Canis::App& _app) 
{
    DEFAULT_CONFIG_AND_REQUIRED(
            healerStateMachineConf,
            AICombat::HealerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

    healerStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::HealerStateMachine);

    _app.RegisterScript(healerStateMachineConf);
} 

void HealerStateMachine::Create() {}

void HealerStateMachine::Ready() {}

void HealerStateMachine::Update(float _dt) {}

void HealerStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);
        transform.position += direction * _speed * _dt;
    }

void HealerStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }



DEFAULT_UNREGISTER_SCRIPT(healerStateMachineConf, HealerStateMachine);

} 
