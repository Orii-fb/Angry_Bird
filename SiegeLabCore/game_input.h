#ifndef SIEGE_LAB_CORE_GAME_INPUT_H_
#define SIEGE_LAB_CORE_GAME_INPUT_H_

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
  int launcher_yaw_axis = 0;
  int launcher_pitch_axis = 0;
  int launcher_speed_axis = 0;
  bool standard_projectile_pressed = false;
  bool heavy_projectile_pressed = false;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_GAME_INPUT_H_
