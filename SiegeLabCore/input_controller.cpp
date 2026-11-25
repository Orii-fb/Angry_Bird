#include "input_controller.h"

namespace siege_lab
{

namespace
{

int ReadAxis(int _positive_key, int _negative_key)
{
  return static_cast<int>(IsKeyDown(_positive_key)) -
         static_cast<int>(IsKeyDown(_negative_key));
}

}  // namespace

InputSnapshot InputController::Capture()
{
  InputSnapshot input;
  input.orbit_button_down = IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
  input.mouse_delta = GetMouseDelta();
  input.mouse_wheel = GetMouseWheelMove();
  input.yaw_axis = ReadAxis(KEY_RIGHT, KEY_LEFT);
  input.pitch_axis = ReadAxis(KEY_UP, KEY_DOWN);
  input.zoom_axis = ReadAxis(KEY_KP_ADD, KEY_KP_SUBTRACT);
  input.launcher_yaw_axis = ReadAxis(KEY_D, KEY_A);
  input.launcher_pitch_axis = ReadAxis(KEY_W, KEY_S);
  input.launcher_speed_axis = ReadAxis(KEY_E, KEY_Q);
  input.standard_projectile_pressed = IsKeyPressed(KEY_ONE);
  input.heavy_projectile_pressed = IsKeyPressed(KEY_TWO);
  input.fire_pressed = m_fire_press_tracker.Update(IsKeyDown(KEY_SPACE));
  return input;
}

}  // namespace siege_lab
