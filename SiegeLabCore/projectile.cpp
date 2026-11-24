#include "projectile.h"

#include <cyclone/core.h>

namespace siege_lab
{

namespace
{

constexpr cyclone::real kGravity = -9.81;
constexpr cyclone::real kLinearDamping = 0.99;
constexpr cyclone::real kAngularDamping = 0.8;
constexpr cyclone::real kSphereInertiaFactor = 0.4;

}  // namespace

Projectile::Projectile()
{
  m_sphere.body = &Body();
}

void Projectile::Fire(const ShotSettings& _settings)
{
  const Vector3 velocity = _settings.GetInitialVelocity();
  Body().setPosition(
      _settings.origin.x, _settings.origin.y, _settings.origin.z);
  Body().setVelocity(velocity.x, velocity.y, velocity.z);
  Body().setAwake();
  Body().calculateDerivedData();
  RegisterPhysics();
}

void Projectile::Draw() const
{
  const cyclone::Vector3 position = Body().getPosition();
  DrawSphere(
      ::Vector3{
          static_cast<float>(position.x), static_cast<float>(position.y),
          static_cast<float>(position.z)},
      static_cast<float>(m_sphere.radius), m_color);
}

cyclone::CollisionSphere& Projectile::Collider()
{
  return m_sphere;
}

const cyclone::CollisionSphere& Projectile::Collider() const
{
  return m_sphere;
}

void Projectile::ConfigurePhysics(float _mass, float _radius, Color _color)
{
  m_sphere.radius = _radius;
  m_color = _color;

  const cyclone::real inertia =
      kSphereInertiaFactor * _mass * _radius * _radius;
  cyclone::Matrix3 inertia_tensor;
  inertia_tensor.setDiagonal(inertia, inertia, inertia);
  Body().setMass(_mass);
  Body().setInertiaTensor(inertia_tensor);
  Body().setDamping(kLinearDamping, kAngularDamping);
  Body().setAcceleration(0.0f, kGravity, 0.0f);
  Body().setPosition(0.0f, 0.0f, 0.0f);
  Body().setVelocity(0.0f, 0.0f, 0.0f);
  Body().setRotation(0.0f, 0.0f, 0.0f);
  Body().setAwake();
  Body().calculateDerivedData();
  RegisterPhysics();
}

void Projectile::RegisterPhysics()
{
  m_sphere.calculateInternals();
}

StandardProjectile::StandardProjectile()
{
  ConfigurePhysics(1.0f, 0.28f, ORANGE);
}

float StandardProjectile::Mass() const
{
  return 1.0f;
}

float StandardProjectile::Radius() const
{
  return 0.28f;
}

HeavyProjectile::HeavyProjectile()
{
  ConfigurePhysics(2.0f, 0.36f, BLUE);
}

float HeavyProjectile::Mass() const
{
  return 2.0f;
}

float HeavyProjectile::Radius() const
{
  return 0.36f;
}

}  // namespace siege_lab
