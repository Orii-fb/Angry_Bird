#include <cmath>

#include <gtest/gtest.h>

#include "camera_manager.h"
#include "orbit_camera.h"

namespace siege_lab
{

namespace
{

bool Near(float _left, float _right)
{
  return std::fabs(_left - _right) < 0.01f;
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

TEST(OrbitCameraTest, StartsAtPositiveZAndLooksAtTarget)
{
  OrbitCamera camera({2.0f, 1.5f, 0.0f}, 10.0f, 90.0f);

  const Camera3D& pose = camera.GetCamera();
  EXPECT_TRUE(Near(pose.position.x, 2.0f));
  EXPECT_TRUE(Near(pose.position.y, 1.5f));
  EXPECT_TRUE(Near(pose.position.z, 10.0f));
  EXPECT_TRUE(Near(pose.target.x, 2.0f));
  EXPECT_TRUE(Near(pose.target.y, 1.5f));
  EXPECT_TRUE(Near(pose.target.z, 0.0f));
}

TEST(OrbitCameraTest, MouseDragOrbitsWithoutDeltaTimeScaling)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.orbit_button_down = true;
  input.mouse_delta = {100.0f, -50.0f};

  camera.Update(input, 0.01f);
  const Camera3D& pose = camera.GetCamera();

  EXPECT_TRUE(Near(pose.position.x, 10.0f * std::sin(25.0f * PI / 180.0f) *
                                         std::cos(12.5f * PI / 180.0f)));
  EXPECT_TRUE(Near(pose.position.y, 10.0f * std::sin(12.5f * PI / 180.0f)));
  EXPECT_TRUE(Near(pose.position.z, 10.0f * std::cos(25.0f * PI / 180.0f) *
                                         std::cos(12.5f * PI / 180.0f)));
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

TEST(OrbitCameraTest, KeyboardOrbitUsesRotationSpeedAndDeltaTime)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.yaw_axis = 1;

  camera.Update(input, 1.0f);
  const Camera3D& pose = camera.GetCamera();

  EXPECT_TRUE(Near(pose.position.x, 10.0f));
  EXPECT_TRUE(Near(pose.position.z, 0.0f));
}

TEST(OrbitCameraTest, KeyboardZoomUsesDeltaTime)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.zoom_axis = 1;

  camera.Update(input, 1.0f);

  EXPECT_TRUE(Near(camera.GetCamera().position.z, 5.0f));
}

TEST(OrbitCameraTest, ClampsPitchAndZoom)
{
  OrbitCamera camera({0.0f, 0.0f, 0.0f}, 10.0f, 90.0f);
  InputSnapshot input;
  input.orbit_button_down = true;
  input.mouse_delta = {0.0f, -10000.0f};
  input.mouse_wheel = 100.0f;

  camera.Update(input, 1.0f);
  const Camera3D& pose = camera.GetCamera();

  EXPECT_TRUE(Near(pose.position.y, 2.0f * std::sin(89.0f * PI / 180.0f)));
  EXPECT_TRUE(Near(std::sqrt(
      pose.position.x * pose.position.x +
      pose.position.y * pose.position.y +
      pose.position.z * pose.position.z), 2.0f));
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
