// Clough42 Electronic Leadscrew
// https://github.com/clough42/electronic-leadscrew
//
// MIT License
//
// Copyright (c) 2019 James Clough
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __STEPPERDRIVE_H
#define __STEPPERDRIVE_H

#include "Configuration.h"
#include "F28x_Project.h"

#define STEP_PIN        GPIO0
#define DIRECTION_PIN   GPIO1
#define ENABLE_PIN      GPIO6
#define ALARM_PIN       GPIO7

#define GPIO_SET(pin)   GpioDataRegs.GPASET.bit.pin = 1
#define GPIO_CLEAR(pin) GpioDataRegs.GPACLEAR.bit.pin = 1
#define GPIO_GET(pin)   GpioDataRegs.GPADAT.bit.pin

#ifdef INVERT_STEP_PIN
#define GPIO_SET_STEP   GPIO_CLEAR(STEP_PIN)
#define GPIO_CLEAR_STEP GPIO_SET(STEP_PIN)
#else
#define GPIO_SET_STEP   GPIO_SET(STEP_PIN)
#define GPIO_CLEAR_STEP GPIO_CLEAR(STEP_PIN)
#endif

#ifdef INVERT_DIRECTION_PIN
#define GPIO_SET_DIRECTION   GPIO_CLEAR(DIRECTION_PIN)
#define GPIO_CLEAR_DIRECTION GPIO_SET(DIRECTION_PIN)
#else
#define GPIO_SET_DIRECTION   GPIO_SET(DIRECTION_PIN)
#define GPIO_CLEAR_DIRECTION GPIO_CLEAR(DIRECTION_PIN)
#endif

#ifdef INVERT_ENABLE_PIN
#define GPIO_SET_ENABLE   GPIO_CLEAR(ENABLE_PIN)
#define GPIO_CLEAR_ENABLE GPIO_SET(ENABLE_PIN)
#else
#define GPIO_SET_ENABLE   GPIO_SET(ENABLE_PIN)
#define GPIO_CLEAR_ENABLE GPIO_CLEAR(ENABLE_PIN)
#endif

#ifdef INVERT_ALARM_PIN
#define GPIO_GET_ALARM (GPIO_GET(ALARM_PIN) == 0)
#else
#define GPIO_GET_ALARM (GPIO_GET(ALARM_PIN) != 0)
#endif

class StepperDrive {
private:
  //
  // Current position of the motor, in steps
  //
  int32 currentPosition;

  //
  // Desired position of the motor, in steps
  //
  int32 desiredPosition;

  // for when threading to a shoulder
  bool threadingToShoulder;
  bool movingToStart;
  bool holdAtShoulder;
  int32 shoulderPosition;
  int32 startPosition;
  int32 directionToShoulder;

  Uint32 moveToStartDelay;
  Uint32 moveToStartSpeed;
  int accelTime;

  //
  // current state-machine state
  // bit 0 - step signal
  // bit 1 - direction signal
  //
  Uint16 state;

  // Limit Switch State
  Uint16 limitSwState;
  // Previous spindle direction for limit switch release action
  bool prevSpindleDir;
  // Save previus difference sign for LimitSw operation. true > 0 ; false < 0.
  bool prevDiffSignPositive;
  // Steps per spindle revolution, based on the current feed
  float stepsPerUnitPitch;
  // Thread/feed status from user interface. 1: Thread ; 0: Feed
  bool thread;

  //
  // Is the drive enabled?
  //
  bool enabled;

  // Limit switch interrupt indication
  bool limitSwitchInt;

public:
  StepperDrive(void);
  void initHardware(void);

  bool shoulderISR(int32 diff);
  bool limitSwitchISR(int32 diff, Uint16 rpm, bool spindleDirection);
  void limitSwReached(void) { this->limitSwitchInt = true; }
  void setStepsPerUnitPitch(float stepsPerUnitPitch) {
    this->stepsPerUnitPitch = stepsPerUnitPitch;
  }
  bool getLimitSw() { return this->limitSwitchInt; }
  Uint16 getLimitSwState() { return this->limitSwState; }
  void beginThreadToShoulder(bool start);
  void moveToStart(int32 stepsPerUnitPitch);
  void setShoulder(void) { this->shoulderPosition = currentPosition; }
  void setStart(void) { this->startPosition = this->currentPosition; }
  void setStartOffset(int32 startOffset);
  void setThread(bool thread) { this->thread = thread; }
  bool isAtShoulder(void);
  bool isAtStart(void);
  void resetToShoulder(void);
  int32 getDistanceToShoulder(void) { return desiredPosition - shoulderPosition; }

  void setDesiredPosition(int32 steps) { this->desiredPosition = steps; }
  void incrementCurrentPosition(int32 increment);
  void setCurrentPosition(int32 position) { this->currentPosition = position; }

  bool checkStepBacklog();

  void setEnabled(bool);

  bool isAlarm();

  void ISR(Uint16 rpm, bool spindleDirection);
};

inline void StepperDrive::incrementCurrentPosition(int32 increment) {
  this->currentPosition += increment;
  this->startPosition += increment;
  this->shoulderPosition += increment;
}

inline bool StepperDrive::checkStepBacklog() {
  // holding and retracting are special cases where the backlog can exceed limits (this shouldn't
  // matter since the motor is either stopped or moving at a safe value).
  if (!(holdAtShoulder || movingToStart || limitSwitchInt)) {
    if (abs(this->desiredPosition - this->currentPosition) > MAX_BUFFERED_STEPS) {
      setEnabled(false);
      return true;
    }
  }
  return false;
}

inline void StepperDrive::setEnabled(bool enabled) {
  this->enabled = enabled;
  if (this->enabled) {
    GPIO_SET_ENABLE;
  } else {
    GPIO_CLEAR_ENABLE;
  }
}

inline bool StepperDrive::isAlarm() {
#ifdef USE_ALARM_PIN
  return GPIO_GET_ALARM;
#else
  return false;
#endif
}

inline bool StepperDrive::isAtShoulder() {
  // note: when moving to shoulder currentPosition will stop at the shoulder position
  return (labs(currentPosition - shoulderPosition) <= backlash);
}

inline bool StepperDrive::isAtStart(void) {
  // when moving to start we can have overshot it by the time this function is called so need to
  // allow for this
  if (this->directionToShoulder < 0)
    return (this->currentPosition - this->startPosition) >= -backlash;
  else
    return (this->currentPosition - this->startPosition) <= backlash;
}

inline void StepperDrive::beginThreadToShoulder(bool start) {
  this->threadingToShoulder = start;
  this->directionToShoulder = this->shoulderPosition - this->startPosition;

  // quitting? If so reset current position.
  if (!start)
    this->currentPosition = this->desiredPosition;

  holdAtShoulder = false;
}

// we have a desired position that's behind the shoulder, calculate a currentPosition that moves us
// within 1 thread of the shoulder whilst still maintaining the angular relationship
inline void StepperDrive::resetToShoulder(void) {
  // total steps we've overshot by
  float diff = this->desiredPosition - this->currentPosition;

  // discard the fractional part to get the number of full threads we need to traverse to close up
  // the gap
  int32 ival = diff / this->stepsPerUnitPitch;
  ival       = ival * this->stepsPerUnitPitch;

  this->incrementCurrentPosition(ival);
}

inline void StepperDrive::setStartOffset(int32 startOffset) {
  if (this->directionToShoulder > 0)
    startOffset = -startOffset;

  incrementCurrentPosition(startOffset);
}

// auto retract to start
inline void StepperDrive::moveToStart(int32 stepsPerUnitPitch) {
  int32 diff = this->desiredPosition - this->startPosition;

  int32 ival = diff / stepsPerUnitPitch;
  ival += diff < 0 ? -1 : +1;
  diff = ival * stepsPerUnitPitch;

  this->incrementCurrentPosition(diff);

  moveToStartSpeed = retractSpeed * 10;   // start at 1/5th max speed
  movingToStart    = true;
}

// handle the shoulder during the ISR. Returns true to block stepper movement
inline bool StepperDrive::shoulderISR(int32 diff) {
  // handle threading to shoulder (and auto-retraction to start)
  if (threadingToShoulder) {
    // Only do this on the beginning of a clock pulse (allow pulses that have been started to
    // complete)
    if (this->state < 2) {
      // if we're auto retracting then accelerate the motor to max speed and flag when we get to the
      // start to allow the state machine to continue
      if (movingToStart) {
        int32 dist = labs(diff);

        // rate of acceleration/deceleration
        if (!(++accelTime & 0x1ff)) {
          // if we're nearly at the start then start decelerating
          if (dist < (STEPPER_MICROSTEPS * STEPPER_RESOLUTION / 3)) {
            if (moveToStartSpeed < retractSpeed * 10)
              moveToStartSpeed++;
          } else {
            if (moveToStartSpeed > retractSpeed)
              moveToStartSpeed--;
          }
        }

        // index motor at preset rate
        if (++moveToStartDelay > moveToStartSpeed) {
          moveToStartDelay = 0;

          // when moving to start we've set the desiredPosition to the start so once currentPosition
          // is the same then we're done.
          movingToStart = (dist > backlash);
        } else
          return true;
      } else
      // check if we've reached the shoulder, if so then prevent any further indexing of the stepper
      // note: we use the desiredPosition since this allows manual rotation away from the shoulder
      // to remove the block on motor movement
      {
        int32 dist = getDistanceToShoulder();

        if ((directionToShoulder >= 0 && dist > 0) || (directionToShoulder < 0 && dist < 0)) {
          holdAtShoulder = true;
          return true;
        }

        holdAtShoulder = false;
      }
    }
  }
  return false;
}

// handle the limit switch and his behavior during the ISR. Returns true to block stepper movement
inline bool StepperDrive::limitSwitchISR(int32 diff, Uint16 rpm, bool spindleDirection) {
  // Check if the Limit Switch is active
  if (this->limitSwitchInt) {
    // Turn on the led indicator
    GPIO_WritePin(LED_5, 0);
    // Only do this when it's enabled and on the beginning of a clock pulse (allow pulses that have
    // been started to complete)
    if ((this->state < 2) && this->enabled) {
      // Check the current Status between Thread or Feed mode, the behavior
      // of the limit switch is sligthly different
      if (this->thread) {
        // In Thread mode the movement is stopped and it waits until the spindle it's also stopped
        // or reversed, then substract and integer number of spindle evolutions to close the gap.
        // Finally waits until the diff is 0 or changed the sign to finish the limit siwtch
        // operation.
        switch (this->limitSwState) {
          case 0:
            if (rpm == 0) {
              // Spindle is stoped so probably is a manual aproximation
              // go to state 1 to indicate limitSw on user interface
              this->limitSwState = 1;
            } else {
              // Limit reached in thread operation
              // Save current spindle direction
              this->prevSpindleDir = spindleDirection;
              // go to next state
              this->limitSwState = 2;
            }
            break;
          case 1:
            // Wait until RPM is different to 0 or the limitSw is removed
            if (rpm != 0) {
              // go to the first state
              this->limitSwState = 0;
            } else if (GPIO_ReadPin(LIMIT_SW_GPIO) == 1) {
              // Limit Sw released, so go back to state 0 and clear the limitSw indication
              this->limitSwitchInt = false;
              this->limitSwState   = 0;
              // Allow the movement again
              return false;
            }
            break;

          case 2:
            // Wait until the spindle is stoped
            // Or detect if there si a change in the spindle direction
            if ((rpm == 0) || (spindleDirection != this->prevSpindleDir)) {
              // Close the gap between the desired steps count and the current position
              this->resetToShoulder();
              prevDiffSignPositive = (diff > 0) ? true : false;
              // go to next state
              this->limitSwState = 3;
            }
            break;

          case 3:
            // Spindle is stoped or in the oposite direction, so wait until
            // the difference is 0 or a change in the diff sign
            if ((diff == 0) || (diff < 0 && prevDiffSignPositive)
                || (diff > 0 && !prevDiffSignPositive)) {
              // if (labs(diff) < 10) {
              //  Save current spindle direction
              this->prevSpindleDir = spindleDirection;
              // Go to wait until the next direction invertion
              this->limitSwState = 4;
              // Allow the movement again
              return false;
            }
            break;

          case 4:
            // Wait until the spindle is stoped
            // Or detect if there si a change in the spindle direction
            if (spindleDirection != this->prevSpindleDir) {
              // Reset the limit switch operation
              this->limitSwitchInt = false;
              this->limitSwState   = 0;
            }
            // Allow the movement again
            return false;
            break;
        }
        // disable movement
        return true;
      }
      // In Feed mode the Limit Switch stops the movement and only waits until the input is
      // released, that mean the limit switch unlocked. Thre is no need of synchronization with the
      // spindle because is not a Thread.
      else {
        switch (this->limitSwState) {
          case 0:
            // go to next state
            this->limitSwState = 1;
            break;
          case 1:
            // resinchronize the position while it's stoped
            this->currentPosition = this->desiredPosition;
            if (GPIO_ReadPin(LIMIT_SW_GPIO) == 1) {
              // Clear the limit switch operation
              this->limitSwitchInt = false;
              this->limitSwState   = 0;
              // Allow the movement again
              return false;
            }
        }
        // disable movement
        return true;
      }
    } else if (GPIO_ReadPin(LIMIT_SW_GPIO) == 1) {
      // Clear the limit switch operation
      this->limitSwitchInt = false;
    }
  }
  // Allow movement
  return false;
}

inline void StepperDrive::ISR(Uint16 rpm, bool spindleDirection) {
  int32 diff = this->desiredPosition - this->currentPosition;

  // Check if Thread to Shoulder is being used and takes the control
  if (shoulderISR(diff))
    return;

  // Check if the Limit-Switch has been reached
  if (limitSwitchISR(diff, rpm, spindleDirection))
    return;
  // Turn-off the indicator led.
  if (GPIO_ReadPin(LIMIT_SW_GPIO) == 1)
    GPIO_WritePin(LED_5, 1);

  // generate step
  if (this->enabled) {
    switch (this->state) {
      case 0:
        // Step = 0; Dir = 0
        if (diff <= -backlash) {
          GPIO_SET_STEP;
          this->state = 2;
        } else if (diff >= backlash) {
          GPIO_SET_DIRECTION;
          this->state = 1;
        }
        break;

      case 1:
        // Step = 0; Dir = 1
        if (diff >= backlash) {
          GPIO_SET_STEP;
          this->state = 3;
        } else if (diff <= -backlash) {
          GPIO_CLEAR_DIRECTION;
          this->state = 0;
        }
        break;

      case 2:
        // Step = 1; Dir = 0
        GPIO_CLEAR_STEP;
        this->currentPosition--;
        this->state = 0;
        break;

      case 3:
        // Step = 1; Dir = 1
        GPIO_CLEAR_STEP;
        this->currentPosition++;
        this->state = 1;
        break;
    }
  } else {
    // not enabled; just keep current position in sync
    this->currentPosition = this->desiredPosition;
  }
}

#endif   // __STEPPERDRIVE_H
