#include "game_controller.h"

#include "raylib.h"

#include "camera_manager.h"
#include "input_controller.h"
#include "launcher.h"
#include "orbit_camera.h"

namespace siege_lab
{

namespace
{

void DrawHud(const ShotSettings& _shot)
{
  const char* projectile_name =
      _shot.projectile_kind == ProjectileKind::kStandard ? "standard" : "lourd";

  DrawText("Siege Lab - lanceur", 20, 20, 24, DARKBLUE);
  DrawText("A/D azimut | W/S elevation | Q/E vitesse | 1/2 projectile",
           20, 55, 18, DARKGRAY);
  DrawText("Camera: clic droit + souris, molette | fleches pour tourner",
           20, 80, 18, DARKGRAY);
  DrawText(TextFormat("Vitesse : %.1f m/s | Projectile : %s",
                      _shot.speed, projectile_name),
           20, 105, 18, DARKGRAY);
}

void DrawScene(
    const Camera3D& _camera, const ShotSettings& _shot,
    Vector3 _aim_end)
{
  BeginDrawing();
  ClearBackground(RAYWHITE);
  BeginMode3D(_camera);
  DrawGrid(20, 1.0f);
  DrawSphere(_shot.origin, 0.25f, DARKBLUE);
  DrawLine3D(_shot.origin, _aim_end, RED);
  EndMode3D();
  DrawHud(_shot);
  EndDrawing();
}

}  // namespace

void GameController::Run()
{
  InitWindow(1280, 720, "Siege Lab");
  SetTargetFPS(60);

  OrbitCamera orbit_camera({2.0f, 1.5f, 0.0f}, 12.0f, 90.0f);
  CameraManager camera_manager(orbit_camera);
  InputController input_controller;
  Launcher launcher;

  while (!WindowShouldClose())
  {
    const InputSnapshot input = input_controller.Capture();
    const float delta_time = GetFrameTime();
    camera_manager.Update(input, delta_time);
    launcher.Aim(input, delta_time);
    const ShotSettings shot = launcher.GetShotSettings();
    const float preview_distance = shot.speed * 0.3f;
    const Vector3 aim_end{
        shot.origin.x + shot.direction.x * preview_distance,
        shot.origin.y + shot.direction.y * preview_distance,
        shot.origin.z + shot.direction.z * preview_distance};
    DrawScene(camera_manager.GetCamera(), shot, aim_end);
  }

  CloseWindow();
}

}  // namespace siege_lab
