#ifndef SIEGE_LAB_CORE_PROJECTILE_H_
#define SIEGE_LAB_CORE_PROJECTILE_H_

#include <cyclone/collide_fine.h>

#include "game_entity.h"
#include "launcher.h"

namespace siege_lab
{

class Projectile : public GameEntity
{
 public:
  void Fire(const ShotSettings& _settings);
  void Draw() const override;
  void RegisterPhysics() override;

  cyclone::CollisionSphere& Collider();
  const cyclone::CollisionSphere& Collider() const;

  virtual float Mass() const = 0;
  virtual float Radius() const = 0;

 protected:
  Projectile();
  void ConfigurePhysics(float _mass, float _radius, Color _color);

 private:
  cyclone::CollisionSphere m_sphere;
  Color m_color{255, 255, 255, 255};
};

class StandardProjectile final : public Projectile
{
 public:
  StandardProjectile();
  float Mass() const override;
  float Radius() const override;
};

class HeavyProjectile final : public Projectile
{
 public:
  HeavyProjectile();
  float Mass() const override;
  float Radius() const override;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_PROJECTILE_H_
