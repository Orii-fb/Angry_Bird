#ifndef SIEGE_LAB_CORE_CAMERA_CONTROLLER_H_
#define SIEGE_LAB_CORE_CAMERA_CONTROLLER_H_

#include "raylib.h"

namespace siege_lab
{

struct InputSnapshot
{
  bool orbit_button_down = false;
  Vector2 mouse_delta{0.0f, 0.0f};
  float mouse_wheel = 0.0f;
  int yaw_axis = 0;
  int pitch_axis = 0;
  int zoom_axis = 0;
};

class ICameraController
{
 public:
  virtual ~ICameraController() = default;
  virtual void Update(const InputSnapshot& _input, float _delta_time) = 0;
  virtual const Camera3D& GetCamera() const = 0;
  virtual void Reset() = 0;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_CAMERA_CONTROLLER_H_
