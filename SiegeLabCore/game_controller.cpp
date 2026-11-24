#include "game_controller.h"

#include "raylib.h"

namespace siege_lab
{

void GameController::Run()
{
  InitWindow(800, 600, "Siege Lab");
  SetTargetFPS(60);

  while (!WindowShouldClose())
  {
    BeginDrawing();
    ClearBackground(RAYWHITE);
    DrawText("Siege Lab", 20, 20, 32, DARKBLUE);
    DrawText("Setup complete - press ESC to exit", 20, 70, 20, DARKGRAY);
    EndDrawing();
  }

  CloseWindow();
}

}  // namespace siege_lab
