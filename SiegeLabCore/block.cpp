#include "block.h"

#include <cyclone/core.h>
#include <raymath.h>
#include <rlgl.h>

namespace siege_lab
{

namespace
{

constexpr cyclone::real kGravity = -9.81;
constexpr cyclone::real kLinearDamping = 0.98;
constexpr cyclone::real kAngularDamping = 0.85;

cyclone::Matrix3 CalculateBoxInertia(
    float _mass, const cyclone::Vector3& _half_size)
{
  const cyclone::real width = 2.0 * _half_size.x;
  const cyclone::real height = 2.0 * _half_size.y;
  const cyclone::real depth = 2.0 * _half_size.z;
  cyclone::Matrix3 inertia;
  inertia.setDiagonal(
      _mass * (height * height + depth * depth) / 12.0,
      _mass * (width * width + depth * depth) / 12.0,
      _mass * (width * width + height * height) / 12.0);
  return inertia;
}

void DrawOrientedBox(
    const cyclone::RigidBody& _body, const cyclone::Vector3& _half_size,
    Color _color)
{
  const cyclone::Vector3 position = _body.getPosition();
  const cyclone::Quaternion orientation = _body.getOrientation();
  const ::Quaternion rotation{
      static_cast<float>(orientation.i), static_cast<float>(orientation.j),
      static_cast<float>(orientation.k), static_cast<float>(orientation.r)};
  const Matrix rotation_matrix = QuaternionToMatrix(rotation);
  const float16 rotation_values = MatrixToFloatV(rotation_matrix);

  rlPushMatrix();
  rlTranslatef(
      static_cast<float>(position.x), static_cast<float>(position.y),
      static_cast<float>(position.z));
  rlMultMatrixf(rotation_values.v);
  DrawCubeV(
      ::Vector3{0.0f, 0.0f, 0.0f},
      ::Vector3{
          static_cast<float>(2.0 * _half_size.x),
          static_cast<float>(2.0 * _half_size.y),
          static_cast<float>(2.0 * _half_size.z)},
      _color);
  rlPopMatrix();
}

}  // namespace

Block::Block(
    float _mass, cyclone::Vector3 _half_size, Vector3 _position, Color _color)
    : m_color(_color)
{
  m_box.body = &Body();
  m_box.halfSize = _half_size;
  Body().setMass(_mass);
  Body().setInertiaTensor(CalculateBoxInertia(_mass, _half_size));
  Body().setDamping(kLinearDamping, kAngularDamping);
  Body().setAcceleration(0.0, kGravity, 0.0);
  Body().setPosition(_position.x, _position.y, _position.z);
  Body().setOrientation(1.0, 0.0, 0.0, 0.0);
  Body().setVelocity(0.0, 0.0, 0.0);
  Body().setRotation(0.0, 0.0, 0.0);
  Body().setAwake(false);
  Body().calculateDerivedData();
  RegisterPhysics();
}

void Block::Draw() const
{
  DrawOrientedBox(Body(), m_box.halfSize, m_color);
}

void Block::RegisterPhysics()
{
  m_box.calculateInternals();
}

const cyclone::CollisionBox& Block::Collider() const
{
  return m_box;
}

cyclone::CollisionBox& Block::Collider()
{
  return m_box;
}

PillarBlock::PillarBlock()
    : Block(
          3.0f, cyclone::Vector3(0.5, 1.5, 0.5),
          Vector3{0.0f, 1.5f, 0.0f}, BLUE)
{
}

CubeBlock::CubeBlock()
    : Block(
          1.5f, cyclone::Vector3(0.5, 0.5, 0.5),
          Vector3{0.0f, 3.5f, 0.0f}, PURPLE)
{
}

}  // namespace siege_lab
