#include <AICombat/HealerStateMachine.hpp>
#include <AICombat/BrawlerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
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
        State(Name, _stateMachine) {

        }

Canis::Entity* HealerStateMachine::FindWoundedTeammate() const {
    Debug::Log("FindWoundedTeammate starts"); 
    if (teamTag.empty()) {
        Debug::Log("teamTag is empty");
        return nullptr;
    }
    else
    {
        Debug::Log("teamTag is not empty");
    }
    
    Debug::Log("FWT");
    
    Canis::Entity* targetToHeal = nullptr;
    // if (!targetToHeal)
    // {
    //     Debug::Log("no target");
    // }
    // else{
    //     Debug::Log("yes target");
    // }

    int lowestHealthFound = 999999;

    // Canis::Vector3 myPos = entity.GetComponent<Canis::Transform>().GetGlobalPosition();

    for (Canis::Entity* ally : entity.scene.GetEntitiesWithTag(teamTag)) {
        Debug::Log("FWT for loop");
        if (!ally || ally == &entity || !ally->active) {
            Debug::Log ("!ally || ally == &entity || !ally->active");
        }
        
        BrawlerStateMachine* brawler = ally->GetScript<BrawlerStateMachine>();
        Debug::Log ("got the brawlerstatemachine script in FWT");
        
        if (brawler && brawler->IsAlive()) {
            Debug::Log ("if brawler is alive");
            int currentHP = brawler->GetCurrentHealth();
            int maxHP = brawler->maxHealth;

            int healAmount = 2;

            int newHP = currentHP + healAmount;
            if (newHP > maxHP) {
                newHP = maxHP;
            }

            brawler -> Heal(newHP);



            
            // if (currentHP < maxHP) { 
                // Debug::Log ("currentHP < maxHP");
                // float dist = glm::distance(myPos, ally->GetComponent<Canis::Transform>().GetGlobalPosition());
                
                // if (dist <= detectionRange) {
                    // if (currentHP < maxHP) {
                        // if (currentHP < lowestHealthFound){
                        //     Debug::Log("");
                            // lowestHealthFound = currentHP;
                            // targetToHeal = ally;
                        // }
                    // }
                // }
            // }
        }
    }
    
    return targetToHeal;
    Debug::Log("FWT return targetToHeal (end)");
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

    m_useFirstHitSfx = true;
}

void HealerStateMachine::Update(float _dt)  
{
    if (!IsAlive())
            return;

    Canis::Entity* target = FindWoundedTeammate();
    if (!target) return;

    if (target) {
        BrawlerStateMachine* brawler = target->GetScript<BrawlerStateMachine>();

        Canis::Transform& targetTrans = target->GetComponent<Canis::Transform>();

        float angle = targetTrans.rotation.y;

        Canis::Vector3 forward = { sin(angle), 0.0f, cos(angle)};

        Canis::Vector3 targetPos = targetTrans.GetGlobalPosition();
        Canis::Vector3 behindPos = targetTrans.GetGlobalPosition() - (forward * 2.0f);

        MoveTowards(behindPos, moveSpeed, _dt);

        m_healAccumulator += 2.0f * _dt;
        if (m_healAccumulator >= 1.0f) {
            int healAmount = static_cast<int>(m_healAccumulator);
            brawler->Heal(healAmount);
            m_healAccumulator -= static_cast<float>(healAmount);
        }
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
