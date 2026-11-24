#ifndef SIEGE_LAB_CORE_CAMERA_CONTROLLER_H_
#define SIEGE_LAB_CORE_CAMERA_CONTROLLER_H_

#include "game_input.h"

namespace siege_lab
{

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
