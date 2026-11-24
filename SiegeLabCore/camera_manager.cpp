#include "camera_manager.h"

namespace siege_lab
{

CameraManager::CameraManager(ICameraController& _active_controller)
    : m_active_controller(&_active_controller)
{
}

void CameraManager::SetActive(ICameraController& _controller)
{
  m_active_controller = &_controller;
}

void CameraManager::Update(const InputSnapshot& _input, float _delta_time)
{
  m_active_controller->Update(_input, _delta_time);
}

const Camera3D& CameraManager::GetCamera() const
{
  return m_active_controller->GetCamera();
}

void CameraManager::Reset()
{
  m_active_controller->Reset();
}

}  // namespace siege_lab
