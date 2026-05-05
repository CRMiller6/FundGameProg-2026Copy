#include <SuperPupUtilities/MageBlast.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>

#include <algorithm>
#include <string>

namespace SuperPupUtilities
{
    namespace
    {
        YAML::Node EncodeTargetTags(const std::vector<std::string>& _tags)
        {
            YAML::Node node(YAML::NodeType::Sequence);
            for (const std::string& tag : _tags)
                node.push_back(tag);
            return node;
        }

        std::vector<std::string> DecodeTargetTags(const YAML::Node& _node)
        {
            std::vector<std::string> tags = {};
            if (!_node || !_node.IsSequence())
                return tags;

            for (const YAML::Node& tagNode : _node)
                tags.push_back(tagNode.as<std::string>(""));

            return tags;
        }
    }

    Canis::ScriptConf mageBlastConf = {};

    void RegisterMageBlastScript(Canis::App& _app)
    {
        DEFAULT_CONFIG_AND_REQUIRED(mageBlastConf, SuperPupUtilities::MageBlast, Canis::Transform);

        mageBlastConf.Encode = [](YAML::Node& _node, Canis::Entity& _entity) -> void
        {
            MageBlast* component = _entity.GetScript<MageBlast>();
            if (component == nullptr)
                return;

            YAML::Node mageBlastNode;
            mageBlastNode["damage"] = component->damage;
            mageBlastNode["speed"] = component->speed;
            mageBlastNode["lifeTime"] = component->lifeTime;
            mageBlastNode["gravity"] = component->gravity;
            mageBlastNode["destroyOnImpact"] = component->destroyOnImpact;
            mageBlastNode["destroyEntityWhenDone"] = component->destroyEntityWhenDone;
            mageBlastNode["autoLaunch"] = component->autoLaunch;
            mageBlastNode["collisionMask"] = component->collisionMask;
            mageBlastNode["hitImpulse"] = component->hitImpulse;
            mageBlastNode["targetTags"] = EncodeTargetTags(component->targetTags);
            _node[MageBlast::ScriptName] = mageBlastNode;
        };

        mageBlastConf.Decode = [](YAML::Node& _node, Canis::Entity& _entity, bool _callCreate) -> void
        {
            YAML::Node mageBlastNode = _node[MageBlast::ScriptName];
            if (!mageBlastNode)
                return;

            MageBlast* component = _entity.GetScript<MageBlast>();
            if (component == nullptr)
                component = _entity.AddScript<MageBlast>(_callCreate);

            if (component == nullptr)
                return;

            component->damage = mageBlastNode["damage"].as<int>(component->damage);
            component->speed = mageBlastNode["speed"].as<float>(component->speed);
            component->lifeTime = mageBlastNode["lifeTime"].as<float>(component->lifeTime);
            component->gravity = mageBlastNode["gravity"].as<float>(component->gravity);
            component->destroyOnImpact = mageBlastNode["destroyOnImpact"].as<bool>(component->destroyOnImpact);
            component->destroyEntityWhenDone = mageBlastNode["destroyEntityWhenDone"].as<bool>(component->destroyEntityWhenDone);
            component->autoLaunch = mageBlastNode["autoLaunch"].as<bool>(component->autoLaunch);
            component->collisionMask = mageBlastNode["collisionMask"].as<Canis::Mask>(component->collisionMask);
            component->hitImpulse = mageBlastNode["hitImpulse"].as<float>(component->hitImpulse);
            component->targetTags = DecodeTargetTags(mageBlastNode["targetTags"]);
        };

        mageBlastConf.DrawInspector = [](Canis::Editor& _editor, Canis::Entity& _entity, const Canis::ScriptConf& _conf) -> void
        {
            MageBlast* component = _entity.GetScript<MageBlast>();
            if (component == nullptr)
                return;

            DrawInspectorField(_editor, "damage", _conf.name.c_str(), component->damage);
            DrawInspectorField(_editor, "speed", _conf.name.c_str(), component->speed);
            DrawInspectorField(_editor, "lifeTime", _conf.name.c_str(), component->lifeTime);
            DrawInspectorField(_editor, "gravity", _conf.name.c_str(), component->gravity);
            DrawInspectorField(_editor, "destroyOnImpact", _conf.name.c_str(), component->destroyOnImpact);
            DrawInspectorField(_editor, "destroyEntityWhenDone", _conf.name.c_str(), component->destroyEntityWhenDone);
            DrawInspectorField(_editor, "autoLaunch", _conf.name.c_str(), component->autoLaunch);
            DrawInspectorField(_editor, "collisionMask", _conf.name.c_str(), component->collisionMask);
            DrawInspectorField(_editor, "hitImpulse", _conf.name.c_str(), component->hitImpulse);

            if (ImGui::Button(("Add Target Tag##" + _conf.name).c_str()))
                component->targetTags.emplace_back();

            for (size_t i = 0; i < component->targetTags.size(); ++i)
            {
                std::string& tag = component->targetTags[i];
                const std::string indexId = _conf.name + std::to_string(i);
                DrawInspectorField(_editor, "targetTag", indexId.c_str(), tag);

                if (ImGui::Button(("Remove##tag" + indexId).c_str()))
                {
                    component->targetTags.erase(component->targetTags.begin() + i);
                    --i;
                }
            }

            ImGui::Text("Launched: %s", component->IsLaunched() ? "Yes" : "No");
        };

        _app.RegisterScript(mageBlastConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(mageBlastConf, MageBlast)

    void MageBlast::Create()
    {
        entity.GetComponent<Canis::Transform>();
    }

    void MageBlast::Ready()
    {
        ResetLifetime();

        if (autoLaunch && entity.active)
            Launch();
    }

    void MageBlast::Destroy() {}

    void MageBlast::Update(float _dt)
    {
        if (!m_launched || !entity.active || !entity.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 start = m_lastPosition;

        Move(_dt);

        const Canis::Vector3 end = transform.GetGlobalPosition();
        CollisionCheck(start, end);

        if (!entity.active)
            return;

        m_lastPosition = end;
        m_timeRemaining -= _dt;

        if (m_timeRemaining <= 0.0f)
            DestroyMageBlast();
    }

    void MageBlast::Launch()
    {
        if (!entity.HasComponent<Canis::Transform>())
            return;

        ResetLifetime();
        m_launched = true;
        m_lastPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        entity.active = true;
    }

    void MageBlast::Launch(const Canis::Vector3& _position, const Canis::Vector3& _rotation)
    {
        if (!entity.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        transform.position = _position;
        transform.rotation = _rotation;
        Launch();
    }

    void MageBlast::DestroyMageBlast()
    {
        m_launched = false;
        ResetLifetime();

        if (destroyEntityWhenDone)
        {
            entity.Destroy();
            return;
        }

        entity.active = false;
    }

    bool MageBlast::IsLaunched() const
    {
        return m_launched;
    }

    void MageBlast::ResetLifetime()
    {
        m_timeRemaining = lifeTime;
    }

    void MageBlast::Move(float _dt)
    {
        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        transform.position += transform.GetForward() * speed * _dt;
        transform.position += Canis::Vector3(0.0f, gravity * _dt, 0.0f);
    }

    void MageBlast::CollisionCheck(const Canis::Vector3& _start, const Canis::Vector3& _end)
    {
        const Canis::Vector3 travel = _end - _start;
        const float distance = glm::length(travel);
        if (distance <= 0.0001f)
            return;

        const Canis::Vector3 direction = travel / distance;
        Canis::RaycastHit hit = {};

        if (!entity.scene.Raycast(_start, direction, hit, distance, collisionMask))
            return;

        if (hit.entity == nullptr || hit.entity == &entity)
            return;

        if (IsValidTarget(*hit.entity) && hitImpulse > 0.0f && hit.entity->HasComponent<Canis::Rigidbody>())
        {
            hit.entity->GetComponent<Canis::Rigidbody>().AddForce(
                direction * hitImpulse,
                Canis::Rigidbody3DForceMode::IMPULSE);
        }

        if (destroyOnImpact)
            DestroyMageBlast();
    }

    bool MageBlast::IsValidTarget(const Canis::Entity& _entity) const
    {
        if (targetTags.empty())
            return true;

        return std::find(targetTags.begin(), targetTags.end(), _entity.tag) != targetTags.end();
    }
}
