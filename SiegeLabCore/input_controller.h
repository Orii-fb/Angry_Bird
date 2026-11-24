#ifndef SIEGE_LAB_CORE_INPUT_CONTROLLER_H_
#define SIEGE_LAB_CORE_INPUT_CONTROLLER_H_

#include "camera_controller.h"

namespace siege_lab
{

class InputController
{
 public:
  InputSnapshot Capture() const;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_INPUT_CONTROLLER_H_
