#include <cmath>

#include <gtest/gtest.h>

#include "camera_manager.h"
#include "launcher.h"
#include "orbit_camera.h"

namespace siege_lab
{

namespace
{

bool Near(float _left, float _right)
{
  return std::fabs(_left - _right) < 0.01f;
}

float Radius(const Camera3D& _camera)
{
  const Vector3 offset{
      _camera.position.x - _camera.target.x,
      _camera.position.y - _camera.target.y,
      _camera.position.z - _camera.target.z};
  return std::sqrt(
      offset.x * offset.x + offset.y * offset.y + offset.z * offset.z);
}

void ExpectPosition(
    const Camera3D& _camera, float _x, float _y, float _z)
{
  EXPECT_NEAR(_x, _camera.position.x, 1e-3f);
  EXPECT_NEAR(_y, _camera.position.y, 1e-3f);
  EXPECT_NEAR(_z, _camera.position.z, 1e-3f);
}

void ApplyMouseFrames(
    OrbitCamera& _camera, int _frames, float _pixels_per_frame)
{
  InputSnapshot input;
  input.orbit_button_down = true;
  input.mouse_delta.x = _pixels_per_frame;
  for (int frame = 0; frame < _frames; ++frame)
  {
    _camera.Update(input, 1.0f / static_cast<float>(_frames));
  }
}

class TestCameraController : public ICameraController
{
 public:
  void Update(const InputSnapshot&, float _delta_time) override
  {
    update_count++;
    last_delta_time = _delta_time;
  }

  const Camera3D& GetCamera() const override
  {
    return m_camera;
  }

  void Reset() override
  {
    reset_called = true;
  }

  Camera3D m_camera{};
  float last_delta_time = 0.0f;
  int update_count = 0;
  bool reset_called = false;
};

}  // namespace

TEST(Camera, C01_TargetAndRadiusRemainStable)
{
  OrbitCamera camera({2.0f, 1.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.pitch_axis = 1;
  camera.Update(input, 20.0f / 90.0f);

  const Camera3D initial = camera.GetCamera();
  ExpectPosition(initial, 2.0f, 4.4202f, 9.3969f);

  input.pitch_axis = 1;
  camera.Update(input, 10.0f / 90.0f);
  input.pitch_axis = 0;
  input.yaw_axis = 1;
  camera.Update(input, 1.0f);
  const Camera3D final = camera.GetCamera();
  ExpectPosition(final, 10.6603f, 6.0f, 0.0f);
  EXPECT_NEAR(2.0f, final.target.x, 1e-3f);
  EXPECT_NEAR(1.0f, final.target.y, 1e-3f);
  EXPECT_NEAR(0.0f, final.target.z, 1e-3f);
  EXPECT_NEAR(10.0f, Radius(final), 1e-3f);
}

TEST(Camera, C02_PitchAndZoomAreClamped)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.orbit_button_down = true;
  input.mouse_delta = {0.0f, -1000.0f};
  input.mouse_wheel = 100.0f;

  camera.Update(input, 0.0f);
  const Camera3D upper = camera.GetCamera();
  EXPECT_NEAR(2.0f, Radius(upper), 1e-3f);
  EXPECT_GT(upper.position.y, 1.99f);

  input.mouse_delta.y = 1000.0f;
  input.mouse_wheel = -100.0f;
  camera.Update(input, 0.0f);
  const Camera3D lower = camera.GetCamera();
  EXPECT_NEAR(50.0f, Radius(lower), 1e-3f);
  EXPECT_LT(lower.position.y, -49.9f);
}

TEST(OrbitCameraTest, IgnoresMouseMovementWithoutRightButton)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.mouse_delta = {100.0f, -50.0f};

  camera.Update(input, 0.0f);

  EXPECT_TRUE(Near(camera.GetCamera().position.x, 0.0f));
  EXPECT_TRUE(Near(camera.GetCamera().position.y, 0.0f));
  EXPECT_TRUE(Near(camera.GetCamera().position.z, 10.0f));
}

TEST(Camera, C03_FullTurnReturnsToSamePose)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.yaw_axis = 1;

  camera.Update(input, 1.0f / 3.0f);
  const Vector3 before = camera.GetCamera().position;
  camera.Update(input, 4.0f);
  const Vector3 after = camera.GetCamera().position;

  EXPECT_NEAR(before.x, after.x, 1e-4f);
  EXPECT_NEAR(before.y, after.y, 1e-4f);
  EXPECT_NEAR(before.z, after.z, 1e-4f);
}

TEST(Camera, C04_RotationSpeedUsesDeltaTime)
{
  OrbitCamera half_second({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  OrbitCamera split_updates({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.yaw_axis = 1;

  half_second.Update(input, 0.5f);
  split_updates.Update(input, 0.25f);
  split_updates.Update(input, 0.25f);

  const Vector3 first = half_second.GetCamera().position;
  const Vector3 second = split_updates.GetCamera().position;
  EXPECT_NEAR(7.0711f, first.x, 1e-3f);
  EXPECT_NEAR(7.0711f, first.z, 1e-3f);
  EXPECT_NEAR(first.x, second.x, 1e-3f);
  EXPECT_NEAR(first.z, second.z, 1e-3f);
}

TEST(Camera, C05_MouseDragIsFrameRateIndependent)
{
  OrbitCamera sixty_fps({0.0f, 0.0f, 0.0f}, 10.0f, 360.0f);
  OrbitCamera thirty_fps({0.0f, 0.0f, 0.0f}, 10.0f, 360.0f);
  ApplyMouseFrames(sixty_fps, 60, 2.0f);
  ApplyMouseFrames(thirty_fps, 30, 4.0f);

  EXPECT_NEAR(5.0f, sixty_fps.GetCamera().position.x, 1e-3f);
  EXPECT_NEAR(8.6603f, sixty_fps.GetCamera().position.z, 1e-3f);
  EXPECT_NEAR(
      sixty_fps.GetCamera().position.x,
      thirty_fps.GetCamera().position.x, 1e-3f);
  EXPECT_NEAR(
      sixty_fps.GetCamera().position.z,
      thirty_fps.GetCamera().position.z, 1e-3f);
}

TEST(OrbitCameraTest, ClampsInitialRadiusAndZoomsWithoutDragging)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 100.0f, 90.0f);
  InputSnapshot input;
  input.mouse_wheel = -5.0f;

  camera.Update(input, 0.0f);
  EXPECT_TRUE(Near(camera.GetCamera().position.z, 50.0f));

  input.mouse_wheel = 5.0f;
  camera.Update(input, 0.0f);
  EXPECT_TRUE(Near(camera.GetCamera().position.z, 45.0f));
}

TEST(OrbitCameraTest, ResetRestoresInitialView)
{
  OrbitCamera camera({1.0f, 2.0f, 3.0f}, 8.0f, 90.0f);
  InputSnapshot input;
  input.orbit_button_down = true;
  input.mouse_delta = {120.0f, -60.0f};
  camera.Update(input, 0.5f);

  camera.Reset();
  const Camera3D& pose = camera.GetCamera();

  EXPECT_TRUE(Near(pose.position.x, 1.0f));
  EXPECT_TRUE(Near(pose.position.y, 2.0f));
  EXPECT_TRUE(Near(pose.position.z, 11.0f));
}

TEST(CameraManagerTest, SwitchesControllerWithoutConcreteTypeChecks)
{
  TestCameraController first_controller;
  TestCameraController second_controller;
  first_controller.m_camera.position.x = 1.0f;
  second_controller.m_camera.position.x = 2.0f;
  CameraManager manager(first_controller);

  EXPECT_TRUE(Near(manager.GetCamera().position.x, 1.0f));
  manager.SetActive(second_controller);
  EXPECT_TRUE(Near(manager.GetCamera().position.x, 2.0f));
}

TEST(CameraManagerTest, CAM06_ManagerCanReplaceController)
{
  TestCameraController replacement;
  replacement.m_camera.position = {5.0f, 2.0f, 5.0f};
  CameraManager manager(replacement);
  Launcher launcher;
  InputSnapshot input;
  input.launcher_yaw_axis = 1;
  launcher.Aim(input, 20.0f / 60.0f);
  input.launcher_yaw_axis = 0;
  input.launcher_pitch_axis = 1;
  launcher.Aim(input, 25.0f / 60.0f);

  EXPECT_NEAR(5.0f, manager.GetCamera().position.x, 1e-4f);
  EXPECT_NEAR(2.0f, manager.GetCamera().position.y, 1e-4f);
  EXPECT_NEAR(5.0f, manager.GetCamera().position.z, 1e-4f);
  const Vector3 direction = launcher.GetShotSettings().direction;
  EXPECT_NEAR(0.8138f, direction.x, 1e-4f);
  EXPECT_NEAR(0.5000f, direction.y, 1e-4f);
  EXPECT_NEAR(0.2962f, direction.z, 1e-4f);
}

TEST(CameraManagerTest, ForwardsUpdateAndResetToActiveController)
{
  TestCameraController controller;
  CameraManager manager(controller);
  InputSnapshot input;

  manager.Update(input, 0.5f);
  manager.Reset();

  EXPECT_EQ(1, controller.update_count);
  EXPECT_TRUE(Near(controller.last_delta_time, 0.5f));
  EXPECT_TRUE(controller.reset_called);
}

}  // namespace siege_lab
