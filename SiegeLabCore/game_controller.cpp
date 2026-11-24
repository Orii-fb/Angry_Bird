#include "game_controller.h"

#include "raylib.h"

#include "camera_manager.h"
#include "input_controller.h"
#include "orbit_camera.h"

namespace siege_lab
{

void GameController::Run()
{
  InitWindow(1280, 720, "Siege Lab");
  SetTargetFPS(60);

  OrbitCamera orbit_camera({2.0f, 1.5f, 0.0f}, 12.0f, 90.0f);
  CameraManager camera_manager(orbit_camera);
  InputController input_controller;

  while (!WindowShouldClose())
  {
    camera_manager.Update(input_controller.Capture(), GetFrameTime());
    BeginDrawing();
    ClearBackground(RAYWHITE);
    BeginMode3D(camera_manager.GetCamera());
    DrawGrid(20, 1.0f);
    EndMode3D();
    DrawText("Siege Lab - camera orbitale", 20, 20, 24, DARKBLUE);
    DrawText("Clic droit + souris : tourner | Molette : zoom", 20, 55, 18,
             DARKGRAY);
    DrawText("Fleches : tourner | Pav. numerique +/- : zoom", 20, 80, 18,
             DARKGRAY);
    EndDrawing();
  }

  CloseWindow();
}

}  // namespace siege_lab
