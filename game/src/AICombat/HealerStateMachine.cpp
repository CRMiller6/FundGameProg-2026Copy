#include <AICombat/HealerStateMachine.hpp>
#include <AICombat/BrawlerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>
#include <algorithm>
#include <cmath>

namespace AICombat {

namespace {
    ScriptConf healerStateMachineConf = {};
}

HealerIdleState::HealerIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

HealState::HealState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {

        }

Canis::Entity* HealerStateMachine::FindWoundedTeammate() const {
    if (healCooldown > 0.0f) {
        return nullptr;
    }

    if (teamTag.empty() || !entity.HasComponent<Canis::Transform>()) {
        return nullptr;
    }

    const Canis::Vector3 healerPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
    Canis::Entity* targetToHeal = nullptr;
    int lowestHealthFound = 999999;

    for (Canis::Entity* ally : entity.scene.GetEntitiesWithTag(teamTag)) {
        if (ally == nullptr || ally == &entity || !ally->active)
            continue;

        if (!ally->HasComponent<Canis::Transform>())
            continue;
        
        BrawlerStateMachine* brawler = ally->GetScript<BrawlerStateMachine>();
        if (brawler == nullptr || !brawler->IsAlive())
            continue;
        
        const int currentHP = brawler->GetCurrentHealth();
        const int maxHP = std::max(brawler->maxHealth, 1);
        if (currentHP >= maxHP)
            continue;

        const Canis::Vector3 allyPosition = ally->GetComponent<Canis::Transform>().GetGlobalPosition();
        if (glm::distance(healerPosition, allyPosition) > detectionRange)
            continue;

        if (currentHP < lowestHealthFound) {
            lowestHealthFound = currentHP;
            targetToHeal = ally;
        }
    }
    
    return targetToHeal;
} 

HealerStateMachine::HealerStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        idleState(*this),
        chaseState(*this),
        healState(*this) {}

void RegisterHealerStateMachineScript(Canis::App& _app) 
{
    // RegisterAccessorProperty(healerStateMachineConf, AICombat::HealerStateMachine, chaseState, moveSpeed);

    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, moveSpeed);
    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, detectionRange);
    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, healAmount);
    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, maxHealth);
    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, teamTag);

    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxPath1);
    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxPath2);
    REGISTER_PROPERTY(healerStateMachineConf, AICombat::HealerStateMachine, hitSfxVolume);
        
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

DEFAULT_UNREGISTER_SCRIPT(healerStateMachineConf, HealerStateMachine);


void HealerStateMachine::Create() {}

void HealerStateMachine::Ready() 
{
    m_currentHealth = std::max(maxHealth, 1);
    m_healAccumulator = 0.0f;
    m_stateTime = 0.0f;
    m_useFirstHitSfx = true;

    if (entity.HasComponent<Canis::Material>())
    {
        m_baseColor = entity.GetComponent<Canis::Material>().color;
        m_hasBaseColor = true;
    }
}

void HealerStateMachine::Update(float _dt)  
{
    if (healCooldown > 0.0f) {
        healCooldown = std::max(0.0f, healCooldown - _dt);
        return;
    }
    
    if (!IsAlive())
        return;

    Canis::Entity* target = FindWoundedTeammate();
    if (target == nullptr) {
        m_healAccumulator = 0.0f;
        return;
    }

    BrawlerStateMachine* brawler = target->GetScript<BrawlerStateMachine>();
    if (brawler == nullptr || !brawler->IsAlive() || !target->HasComponent<Canis::Transform>()) {
        m_healAccumulator = 0.0f;
        return;
    }

    Canis::Transform& targetTrans = target->GetComponent<Canis::Transform>();

    float angle = targetTrans.rotation.y;

    Canis::Vector3 forward = { std::sin(angle), 0.0f, std::cos(angle) };

    Canis::Vector3 behindPos = targetTrans.GetGlobalPosition() - (forward * 2.0f);

    MoveTowards(behindPos, moveSpeed, _dt);

    m_healAccumulator += static_cast<float>(std::max(healAmount, 0)) * _dt;
    if (m_healAccumulator >= 1.0f) {
        const int healingToApply = static_cast<int>(m_healAccumulator);
        brawler->Heal(healingToApply);
        m_healAccumulator -= static_cast<float>(healingToApply);
    }
}

HealerChaseState::HealerChaseState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

void HealerChaseState::Enter() {}

void HealerChaseState::Update(float) {}

void HealerStateMachine::MoveTowards(Canis::Vector3 _targetPos, float _speed, float _dt){
        if (!entity.HasComponent<Canis::Transform>())// || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();

        Canis::Vector3 direction = _targetPos - selfPosition;       //.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f) //length(direction) < 0.1f)
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

    void HealerStateMachine::TakeDamage(int _damage)
    {
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


    void HealerStateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }




    void HealerStateMachine::SpawnDeathEffect() {}

    bool HealerStateMachine::IsAlive() const
    {
        return m_currentHealth > 0;
    }

} 
