#ifndef SIEGE_LAB_CORE_TRAJECTORY_PREDICTOR_H_
#define SIEGE_LAB_CORE_TRAJECTORY_PREDICTOR_H_

#include <span>
#include <vector>

#include <raylib.h>

#include "launcher.h"

namespace siege_lab
{

struct TrajectoryBox
{
  ::Vector3 position;
  ::Vector3 half_size;
  ::Quaternion orientation;
};

struct TrajectoryResult
{
  std::vector<::Vector3> points;
  bool hit_ground = false;
  bool hit_obstacle = false;
  bool reached_step_limit = false;
};

class TrajectoryPredictor
{
 public:
  static constexpr unsigned kDefaultMaximumSteps = 300;

  TrajectoryResult Predict(
      const ShotSettings& _settings,
      std::span<const TrajectoryBox> _obstacles = {},
      unsigned _maximum_steps = kDefaultMaximumSteps) const;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_TRAJECTORY_PREDICTOR_H_
