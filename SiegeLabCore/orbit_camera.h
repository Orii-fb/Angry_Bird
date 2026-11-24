#ifndef SIEGE_LAB_CORE_ORBIT_CAMERA_H_
#define SIEGE_LAB_CORE_ORBIT_CAMERA_H_

#include "camera_controller.h"

namespace siege_lab
{

class OrbitCamera : public ICameraController
{
 public:
  OrbitCamera(Vector3 _target, float _radius,
              float _rotation_speed_degrees_per_second);

  void Update(const InputSnapshot& _input, float _delta_time) override;
  const Camera3D& GetCamera() const override;
  void Reset() override;

 private:
  void ApplyKeyboardRotation(const InputSnapshot& _input, float _delta_time);
  void ApplyMouseRotation(const InputSnapshot& _input);
  void ApplyZoom(const InputSnapshot& _input, float _delta_time);
  void ClampAngles();
  void UpdateCameraPosition();

  Vector3 m_target;
  float m_radius;
  float m_initial_radius;
  float m_rotation_speed_degrees_per_second;
  float m_yaw_degrees = 0.0f;
  float m_pitch_degrees = 0.0f;
  Camera3D m_camera{};
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_ORBIT_CAMERA_H_
