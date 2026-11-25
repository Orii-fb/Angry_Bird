#include "game_controller.h"

#include <algorithm>

#include <raylib.h>

#include "camera_manager.h"
#include "input_controller.h"
#include "launcher.h"
#include "level_builder.h"
#include "orbit_camera.h"
#include "projectile.h"
#include "scene_objects.h"
#include "trajectory_predictor.h"

namespace siege_lab
{

namespace
{

constexpr float kPhysicsStep = 1.0f / 60.0f;
constexpr float kMaximumFrameTime = 0.25f;

Projectile* SpawnProjectile(
    SceneObjects<1>& _projectiles, const ShotSettings& _shot)
{
  std::optional<EntityId> projectile_id;
  if (_shot.projectile_kind == ProjectileKind::kStandard)
  {
    projectile_id = _projectiles.CreateEntity<StandardProjectile>();
  }
  else
  {
    projectile_id = _projectiles.CreateEntity<HeavyProjectile>();
  }

  if (!projectile_id.has_value())
  {
    TraceLog(LOG_ERROR, "Impossible de creer le projectile.");
    return nullptr;
  }

  Projectile* projectile =
      static_cast<Projectile*>(_projectiles.Get(*projectile_id));
  projectile->Fire(_shot);
  return projectile;
}

void UpdateWorld(
    LevelBuilder& _level, SceneObjects<1>& _projectiles,
    Projectile* _projectile,
    float& _accumulator, float _delta_time)
{
  _accumulator += _delta_time;
  while (_accumulator >= kPhysicsStep)
  {
    if (!_level.Step(kPhysicsStep, _projectile))
    {
      TraceLog(LOG_WARNING, "Detection de collisions incomplete.");
    }
    _projectiles.FlushRemovals();
    _accumulator -= kPhysicsStep;
  }
}

void FireIfPressed(
    const InputSnapshot& _input, const ShotSettings& _shot,
    SceneObjects<1>& _projectiles, Projectile*& _active_projectile,
    float& _physics_accumulator);

TrajectoryResult PrepareFrameTrajectory(
    const InputSnapshot& _input, const ShotSettings& _shot,
    TrajectoryPredictor& _predictor, const LevelBuilder& _level,
    SceneObjects<1>& _projectiles, Projectile*& _active_projectile,
    float& _physics_accumulator)
{
  const auto obstacles = _level.GetTrajectoryBoxes();
  const TrajectoryResult trajectory = _predictor.Predict(_shot, obstacles);
  FireIfPressed(
      _input, _shot, _projectiles, _active_projectile, _physics_accumulator);
  return trajectory;
}

void UpdateProjectileWorld(
    LevelBuilder& _level, SceneObjects<1>& _projectiles,
    Projectile*& _active_projectile, float& _physics_accumulator,
    float _delta_time)
{
  UpdateWorld(
      _level, _projectiles, _active_projectile,
      _physics_accumulator, _delta_time);
  if (_active_projectile != nullptr &&
      !_projectiles.Contains(_active_projectile))
  {
    _active_projectile = nullptr;
  }
}

void DrawHud(const ShotSettings& _shot)
{
  const bool heavy = _shot.projectile_kind == ProjectileKind::kHeavy;
  const char* projectile_name = heavy ? "lourd" : "standard";
  const float projectile_mass = heavy ? 2.0f : 1.0f;
  const float projectile_radius = heavy ? 0.36f : 0.28f;

  DrawText("Siege Lab - lanceur", 20, 20, 24, DARKBLUE);
  DrawText("A/D azimut | W/S elevation | Q/E vitesse | 1/2 projectile",
           20, 55, 18, DARKGRAY);
  DrawText("Camera: clic droit + souris, molette | fleches pour tourner",
           20, 80, 18, DARKGRAY);
  DrawText(TextFormat("Vitesse : %.1f m/s | Projectile : %s",
                      _shot.speed, projectile_name),
           20, 105, 18, DARKGRAY);
  DrawText(TextFormat("Profil : %.0f kg | rayon : %.2f m",
                      projectile_mass, projectile_radius),
           20, 130, 18, DARKGRAY);
  DrawText("Espace : tirer", 20, 155, 18, DARKGRAY);
  DrawText("Apercu de trajectoire Cyclone", 20, 180, 18, DARKGREEN);
}

void DrawTrajectory(const TrajectoryResult& _trajectory)
{
  for (std::size_t index = 1; index < _trajectory.points.size(); ++index)
  {
    DrawLine3D(
        _trajectory.points[index - 1], _trajectory.points[index], GREEN);
  }
  if (!_trajectory.points.empty())
  {
    const Color marker_color =
        _trajectory.hit_ground || _trajectory.hit_obstacle ? ORANGE : GREEN;
    DrawSphere(_trajectory.points.back(), 0.07f, marker_color);
  }
}

void DrawScene(
    const Camera3D& _camera, const ShotSettings& _shot,
    const LevelBuilder& _level, const Projectile* _projectile,
    const TrajectoryResult& _trajectory)
{
  BeginDrawing();
  ClearBackground(RAYWHITE);
  BeginMode3D(_camera);
  DrawGrid(20, 1.0f);
  DrawSphere(_shot.origin, 0.25f, DARKBLUE);
  _level.Draw();
  DrawTrajectory(_trajectory);
  if (_projectile != nullptr)
  {
    _projectile->Draw();
  }
  EndMode3D();
  DrawHud(_shot);
  EndDrawing();
}

void FireIfPressed(
    const InputSnapshot& _input, const ShotSettings& _shot,
    SceneObjects<1>& _projectiles, Projectile*& _active_projectile,
    float& _physics_accumulator)
{
  if (!_input.fire_pressed)
  {
    return;
  }
  _projectiles.Clear();
  _active_projectile = SpawnProjectile(_projectiles, _shot);
  _physics_accumulator = 0.0f;
}

void UpdateAndDrawFrame(
    CameraManager& _camera_manager, InputController& _input_controller,
    Launcher& _launcher, TrajectoryPredictor& _predictor,
    LevelBuilder& _level, SceneObjects<1>& _projectiles,
    Projectile*& _active_projectile, float& _physics_accumulator)
{
  const InputSnapshot input = _input_controller.Capture();
  const float delta_time =
      std::clamp(GetFrameTime(), 0.0f, kMaximumFrameTime);
  _camera_manager.Update(input, delta_time);
  _launcher.Aim(input, delta_time);

  const ShotSettings shot = _launcher.GetShotSettings();
  const TrajectoryResult trajectory = PrepareFrameTrajectory(
      input, shot, _predictor, _level, _projectiles,
      _active_projectile, _physics_accumulator);
  UpdateProjectileWorld(
      _level, _projectiles, _active_projectile,
      _physics_accumulator, delta_time);
  DrawScene(
      _camera_manager.GetCamera(), shot, _level, _active_projectile,
      trajectory);
}

bool BuildInitialLevel(LevelBuilder& _level)
{
  if (_level.BuildPillarCube())
  {
    return true;
  }
  TraceLog(LOG_ERROR, "Impossible de creer le niveau pilier et cube.");
  return false;
}

void RunGameLoop(LevelBuilder& _level)
{
  OrbitCamera orbit_camera({2.0f, 1.5f, 0.0f}, 12.0f, 90.0f);
  CameraManager camera_manager(orbit_camera);
  InputController input_controller;
  Launcher launcher;
  TrajectoryPredictor trajectory_predictor;
  SceneObjects<1> projectiles;
  Projectile* active_projectile = nullptr;
  float physics_accumulator = 0.0f;
  while (!WindowShouldClose())
  {
    UpdateAndDrawFrame(
        camera_manager, input_controller, launcher, trajectory_predictor,
        _level, projectiles, active_projectile, physics_accumulator);
  }
}

}  // namespace

void GameController::Run()
{
  InitWindow(1280, 720, "Siege Lab");
  SetTargetFPS(60);

  LevelBuilder level;
  if (!BuildInitialLevel(level))
  {
    CloseWindow();
    return;
  }
  RunGameLoop(level);
  CloseWindow();
}

}  // namespace siege_lab
