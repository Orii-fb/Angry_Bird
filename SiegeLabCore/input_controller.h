#ifndef SIEGE_LAB_CORE_INPUT_CONTROLLER_H_
#define SIEGE_LAB_CORE_INPUT_CONTROLLER_H_

#include "game_input.h"

namespace siege_lab
{

class FirePressTracker
{
 public:
  bool Update(bool _pressed)
  {
    const bool new_press = _pressed && !m_was_pressed;
    m_was_pressed = _pressed;
    return new_press;
  }

 private:
  bool m_was_pressed = false;
};

class InputController
{
 public:
  InputSnapshot Capture();

 private:
  FirePressTracker m_fire_press_tracker;
};

}  // namespace siege_lab

#endif  // SIEGE_LAB_CORE_INPUT_CONTROLLER_H_
