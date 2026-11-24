#include "trajectory_predictor.h"

#include <array>

#include <cyclone/body.h>
#include <cyclone/collide_fine.h>
#include <cyclone/contacts.h>

namespace siege_lab
{

namespace
{

constexpr cyclone::real kPhysicsStep = 1.0 / 60.0;
constexpr cyclone::real kGravity = -9.81;
constexpr cyclone::real kLinearDamping = 0.99;
constexpr cyclone::real kAngularDamping = 0.8;
constexpr cyclone::real kSphereInertiaFactor = 0.4;

struct ProjectileProfile
{
  cyclone::real mass;
  cyclone::real radius;
};

ProjectileProfile GetProfile(ProjectileKind _kind)
{
  if (_kind == ProjectileKind::kHeavy)
  {
    return {2.0, 0.36};
  }
  return {1.0, 0.28};
}

void ConfigureContactData(
    cyclone::CollisionData& _data,
    std::array<cyclone::Contact, 4>& _contacts)
{
  _data.contactArray = _contacts.data();
  _data.friction = 0.8;
  _data.restitution = 0.0;
  _data.tolerance = 0.1;
  _data.reset(static_cast<unsigned>(_contacts.size()));
}

class GhostBody
{
 public:
  explicit GhostBody(const ShotSettings& _settings)
  {
    const ProjectileProfile profile = GetProfile(_settings.projectile_kind);
    const cyclone::real inertia =
        kSphereInertiaFactor * profile.mass * profile.radius * profile.radius;
    cyclone::Matrix3 inertia_tensor;
    inertia_tensor.setDiagonal(inertia, inertia, inertia);
    m_body.setMass(profile.mass);
    m_body.setInertiaTensor(inertia_tensor);
    m_body.setDamping(kLinearDamping, kAngularDamping);
    m_body.setAcceleration(0.0, kGravity, 0.0);
    m_body.setPosition(
        _settings.origin.x, _settings.origin.y, _settings.origin.z);
    const ::Vector3 velocity = _settings.GetInitialVelocity();
    m_body.setVelocity(velocity.x, velocity.y, velocity.z);
    m_body.setAwake();
    m_body.calculateDerivedData();
    m_sphere.body = &m_body;
    m_sphere.radius = profile.radius;
    m_sphere.calculateInternals();
  }

  cyclone::RigidBody& Body()
  {
    return m_body;
  }

  cyclone::CollisionSphere& Collider()
  {
    return m_sphere;
  }

 private:
  cyclone::RigidBody m_body;
  cyclone::CollisionSphere m_sphere;
};

class ObstacleBody
{
 public:
  explicit ObstacleBody(const TrajectoryBox& _snapshot)
  {
    m_body.setInverseMass(0.0);
    m_body.setPosition(
        _snapshot.position.x, _snapshot.position.y, _snapshot.position.z);
    m_body.setRotation(
        _snapshot.rotation.x, _snapshot.rotation.y, _snapshot.rotation.z);
    m_body.calculateDerivedData();
    m_box.body = &m_body;
    m_box.halfSize = cyclone::Vector3(
        _snapshot.half_size.x, _snapshot.half_size.y,
        _snapshot.half_size.z);
    m_box.calculateInternals();
  }

  cyclone::CollisionBox& Collider()
  {
    return m_box;
  }

 private:
  cyclone::RigidBody m_body;
  cyclone::CollisionBox m_box;
};

void BuildObstacleBodies(
    std::span<const TrajectoryBox> _snapshots,
    std::vector<ObstacleBody>& _obstacles)
{
  _obstacles.reserve(_snapshots.size());
  for (const TrajectoryBox& snapshot : _snapshots)
  {
    _obstacles.emplace_back(snapshot);
  }
}

bool FindGroundContact(
    GhostBody& _ghost, cyclone::Contact& _contact)
{
  std::array<cyclone::Contact, 4> contacts;
  cyclone::CollisionData data;
  ConfigureContactData(data, contacts);
  cyclone::CollisionPlane ground;
  ground.direction = cyclone::Vector3(0.0, 1.0, 0.0);
  ground.offset = 0.0;
  if (cyclone::CollisionDetector::sphereAndHalfSpace(
          _ghost.Collider(), ground, &data) == 0)
  {
    return false;
  }
  _contact = contacts[0];
  return true;
}

bool FindObstacleContact(
    GhostBody& _ghost, std::vector<ObstacleBody>& _obstacles,
    cyclone::Contact& _contact)
{
  std::array<cyclone::Contact, 4> contacts;
  cyclone::CollisionData data;
  ConfigureContactData(data, contacts);
  for (ObstacleBody& obstacle : _obstacles)
  {
    data.reset(static_cast<unsigned>(contacts.size()));
    if (cyclone::CollisionDetector::boxAndSphere(
            obstacle.Collider(), _ghost.Collider(), &data) > 0)
    {
      _contact = contacts[0];
      return true;
    }
  }
  return false;
}

cyclone::real GetContactDirection(
    GhostBody& _ghost, const cyclone::Contact& _contact)
{
  if (_contact.body[0] == &_ghost.Body())
  {
    return 1.0;
  }
  if (_contact.body[1] == &_ghost.Body())
  {
    return -1.0;
  }
  return 0.0;
}

void SetGhostAtContact(
    GhostBody& _ghost, const cyclone::Contact& _contact)
{
  const cyclone::real direction = GetContactDirection(_ghost, _contact);
  if (direction == 0.0)
  {
    return;
  }
  const cyclone::Vector3 position = _ghost.Body().getPosition();
  const cyclone::Vector3 normal = _contact.contactNormal;
  const cyclone::real correction = direction * _contact.penetration;
  _ghost.Body().setPosition(
      position.x + normal.x * correction,
      position.y + normal.y * correction,
      position.z + normal.z * correction);
  _ghost.Body().calculateDerivedData();
  _ghost.Collider().calculateInternals();
}

bool DetectContact(
    GhostBody& _ghost, std::vector<ObstacleBody>& _obstacles,
    TrajectoryResult& _result, cyclone::Contact& _contact)
{
  _result.hit_ground = FindGroundContact(_ghost, _contact);
  if (_result.hit_ground)
  {
    SetGhostAtContact(_ghost, _contact);
    return true;
  }

  _result.hit_obstacle =
      FindObstacleContact(_ghost, _obstacles, _contact);
  if (_result.hit_obstacle)
  {
    SetGhostAtContact(_ghost, _contact);
  }
  return _result.hit_obstacle;
}

void AppendPosition(GhostBody& _ghost, TrajectoryResult& _result)
{
  const cyclone::Vector3 position = _ghost.Body().getPosition();
  _result.points.push_back({
      static_cast<float>(position.x), static_cast<float>(position.y),
      static_cast<float>(position.z)});
}

void UpdateLastPosition(GhostBody& _ghost, TrajectoryResult& _result)
{
  const cyclone::Vector3 position = _ghost.Body().getPosition();
  _result.points.back() = {
      static_cast<float>(position.x), static_cast<float>(position.y),
      static_cast<float>(position.z)};
}

bool AdvanceUntilContact(
    GhostBody& _ghost, std::vector<ObstacleBody>& _obstacles,
    TrajectoryResult& _result, unsigned _maximum_steps)
{
  cyclone::Contact contact;
  for (unsigned step = 0; step < _maximum_steps; ++step)
  {
    _ghost.Body().integrate(kPhysicsStep);
    _ghost.Collider().calculateInternals();
    if (DetectContact(_ghost, _obstacles, _result, contact))
    {
      AppendPosition(_ghost, _result);
      return true;
    }
    AppendPosition(_ghost, _result);
  }
  return false;
}

}  // namespace

TrajectoryResult TrajectoryPredictor::Predict(
    const ShotSettings& _settings,
    std::span<const TrajectoryBox> _obstacles,
    unsigned _maximum_steps) const
{
  TrajectoryResult result;
  GhostBody ghost(_settings);
  std::vector<ObstacleBody> obstacles;
  BuildObstacleBodies(_obstacles, obstacles);
  AppendPosition(ghost, result);

  cyclone::Contact contact;
  if (DetectContact(ghost, obstacles, result, contact))
  {
    UpdateLastPosition(ghost, result);
    return result;
  }

  if (!AdvanceUntilContact(ghost, obstacles, result, _maximum_steps))
  {
    result.reached_step_limit = true;
  }

  return result;
}

}  // namespace siege_lab
