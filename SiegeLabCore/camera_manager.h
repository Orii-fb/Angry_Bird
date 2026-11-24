#ifndef SIEGE_LAB_CORE_CAMERA_MANAGER_H_
#define SIEGE_LAB_CORE_CAMERA_MANAGER_H_

#include "camera_controller.h"

namespace siege_lab
{

class CameraManager
{
 public:
  explicit CameraManager(ICameraController& _active_controller);

  void SetActive(ICameraController& _controller);
  void Update(const InputSnapshot& _input, float _delta_time);
  const Camera3D& GetCamera() const;
  void Reset();

 private:
  ICameraController* m_active_controller;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_CAMERA_MANAGER_H_
