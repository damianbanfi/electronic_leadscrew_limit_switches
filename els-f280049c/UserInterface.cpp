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

#include "UserInterface.h"

const MESSAGE STARTUP_MESSAGE_2
    = {.message     = {LETTER_E, LETTER_L, LETTER_S, DASH, ONE | POINT, SIX, BLANK, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 0.5};

const MESSAGE STARTUP_MESSAGE_1
    = {.message     = {LETTER_P, LETTER_A, LETTER_B, LETTER_L, LETTER_O, DASH, TWO, FIVE},
       .displayTime = UI_REFRESH_RATE_HZ * 0.5,
       .next        = &STARTUP_MESSAGE_2};

const MESSAGE CUSTOM_THREAD = {.message = {LETTER_R, LETTER_O, LETTER_S, LETTER_C | POINT, BLANK,
                                           LETTER_E, LETTER_S, LETTER_P | POINT},
                               .displayTime = UI_REFRESH_RATE_HZ * 2.0};

const MESSAGE THREAD_TO_SHOULDER = {.message     = {LETTER_R, LETTER_O, LETTER_S, LETTER_C | POINT,
                                                    LETTER_A, LETTER_U, LETTER_T, LETTER_O},
                                    .displayTime = UI_REFRESH_RATE_HZ * 2.0};

const MESSAGE STOP
    = {.message     = {LETTER_S, LETTER_T, LETTER_O, LETTER_P, BLANK, BLANK, BLANK, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 100.0};

const MESSAGE WAIT
    = {.message     = {LETTER_E, LETTER_S, LETTER_P, LETTER_E, LETTER_R, LETTER_A, LETTER_R, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 1.0};

const MESSAGE GO_SHOULDER
    = {.message     = {BLANK, LETTER_G, LETTER_O, BLANK, BLANK, LETTER_F, LETTER_I, LETTER_N},
       .displayTime = UI_REFRESH_RATE_HZ * 2.0};

const MESSAGE GO_START
    = {.message     = {BLANK, LETTER_G, LETTER_O, BLANK, BLANK, LETTER_I, LETTER_N, LETTER_I},
       .displayTime = UI_REFRESH_RATE_HZ * 2.0};

const MESSAGE RETRACT
    = {.message     = {LETTER_R, LETTER_E, LETTER_T, LETTER_R, LETTER_A, LETTER_E, LETTER_R, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 2.0};

const MESSAGE REVERSE
    = {.message     = {LETTER_R, LETTER_E, LETTER_U, LETTER_E, LETTER_R, LETTER_S, LETTER_E, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 100.0};

const MESSAGE POSITION = {.message     = {LETTER_P, LETTER_O, LETTER_S | POINT, BLANK, LETTER_G,
                                          LETTER_R, LETTER_A, LETTER_D},
                          .displayTime = UI_REFRESH_RATE_HZ * 2.0};

const MESSAGE RPM
    = {.message     = {LETTER_R, LETTER_P, LETTER_M1, LETTER_M2, BLANK, BLANK, BLANK, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 2.0};

const MESSAGE RESET_POS = {.message     = {LETTER_R, LETTER_S, LETTER_T | POINT, BLANK, LETTER_P,
                                           LETTER_O, LETTER_S | POINT, BLANK},
                           .displayTime = UI_REFRESH_RATE_HZ * 2.0};

extern const MESSAGE LIMIT_SW_2;
const MESSAGE LIMIT_SW
    = {.message     = {LETTER_L, LETTER_I, LETTER_M1, LETTER_M2, LETTER_I, LETTER_T, BLANK, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 0.1,
       .next        = &LIMIT_SW_2};

const MESSAGE LIMIT_SW_2 = {.message     = {BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK},
                            .displayTime = UI_REFRESH_RATE_HZ * 0.1,
                            .next        = &LIMIT_SW};

extern const MESSAGE BACKLOG_PANIC_MESSAGE_2;
const MESSAGE BACKLOG_PANIC_MESSAGE_1
    = {.message     = {LETTER_T, LETTER_O, LETTER_O, BLANK, LETTER_F, LETTER_A, LETTER_S, LETTER_T},
       .displayTime = UI_REFRESH_RATE_HZ * .5,
       .next        = &BACKLOG_PANIC_MESSAGE_2};
const MESSAGE BACKLOG_PANIC_MESSAGE_2
    = {.message     = {BLANK, LETTER_R, LETTER_E, LETTER_S, LETTER_E, LETTER_T, BLANK, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * .5,
       .next        = &BACKLOG_PANIC_MESSAGE_1};

// non const messages so we can change the text
MESSAGE MULTI
    = {.message     = {LETTER_M1, LETTER_M2, LETTER_U, LETTER_L, LETTER_T, LETTER_I, BLANK, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 0.2};

MESSAGE BEGIN
    = {.message     = {LETTER_E, LETTER_N, LETTER_T, LETTER_R | POINT, BLANK, BLANK, BLANK, BLANK},
       .displayTime = UI_REFRESH_RATE_HZ * 0.2};

const Uint16 VALUE_BLANK[4] = {BLANK, BLANK, BLANK, BLANK};

UserInterface::UserInterface(ControlPanel *controlPanel, Core *core,
                             FeedTableFactory *feedTableFactory, Encoder *encoder) {
  this->controlPanel     = controlPanel;
  this->core             = core;
  this->feedTableFactory = feedTableFactory;
  this->encoder          = encoder;

  this->metric    = true;    // start out with metric
  this->thread    = false;   // start out with feeds
  this->reverse   = false;   // start out going forward
  this->showAngle = false;   // start out showing RPM

  this->feedTable = NULL;

  this->keys.all = 0xff;

  this->isInMenu = false;

  this->limitSwState = 0;

  // initialize the core so we start up correctly
  core->setReverse(this->reverse);
  core->setFeed(loadFeedTable());

  setMessage(&STARTUP_MESSAGE_1);
}

const FEED_THREAD *UserInterface::loadFeedTable() {
  this->feedTable = this->feedTableFactory->getFeedTable(this->metric, this->thread);
  return this->feedTable->current();
}

LED_REG UserInterface::calculateLEDs() {
  // get the LEDs for this feed
  LED_REG leds = feedTable->current()->leds;

  if (this->core->isPowerOn()) {
    // and add a few of our own
    leds.bit.POWER   = 1;
    leds.bit.REVERSE = this->reverse;
    leds.bit.FORWARD = !this->reverse;
  } else {
    // power is off
    leds.all = 0;
  }

  return leds;
}

void UserInterface::setMessage(const MESSAGE *message) {
  this->message     = message;
  this->messageTime = message->displayTime;
}

void UserInterface::overrideMessage(void) {
  if (this->message != NULL) {
    if (this->messageTime > 0) {
      this->messageTime--;
      controlPanel->setMessage(this->message->message);
    } else {
      this->message = this->message->next;
      if (this->message == NULL)
        controlPanel->setMessage(NULL);
      else
        this->messageTime = this->message->displayTime;
    }
  }
}

void UserInterface::clearMessage(void) {
  this->message     = NULL;
  this->messageTime = 0;
  controlPanel->setMessage(NULL);
}

void UserInterface::panicStepBacklog(void) { setMessage(&BACKLOG_PANIC_MESSAGE_1); }

void UserInterface::loop(void) {
  // read the RPM up front so we can use it to make decisions
  Uint16 currentRpm = core->getRPM();

  // read the current spindle position to keep this up to date
  Uint16 currentSpindleAngle = encoder->getSpindleAngle();

  // display an override message, if there is one
  overrideMessage();

  // clear the mode indication
  controlPanel->showCurMode(BLANK, BLANK);

  // read keypresses from the control panel
  keys = controlPanel->getKeys();

  // handle appropriate loop

  if (isInMenu)
    menuLoop(currentRpm);
  else
    mainLoop(currentRpm);

  // update the control panel
  controlPanel->setLEDs(calculateLEDs());
  controlPanel->setValue(feedTable->current()->display);
  controlPanel->setRPM(currentRpm);
  controlPanel->setSpindleAngle(currentSpindleAngle);

  if (!core->isPowerOn()) {
    controlPanel->setValue(VALUE_BLANK);
  }

  controlPanel->refresh(showAngle);
}

void UserInterface::mainLoop(Uint16 currentRpm) {
  // respond to keypresses
  if (currentRpm == 0) {
    // these keys should only be sensitive when the machine is stopped
    if (keys.bit.POWER) {
      this->core->setPowerOn(!this->core->isPowerOn());
      clearMessage();
    }

    // these should only work when the power is on
    if (this->core->isPowerOn()) {
      if (keys.bit.IN_MM) {
        this->metric = !this->metric;
        core->setFeed(loadFeedTable());
      }
      if (keys.bit.FEED_THREAD) {
        this->thread = !this->thread;
        core->setFeed(loadFeedTable());
        core->setThread(this->thread);
      }
      if (keys.bit.FWD_REV) {
        this->reverse = !this->reverse;
        core->setReverse(this->reverse);
      }
      if (keys.bit.SET) {
        beginMenu(currentRpm);
      }
    }
  }

#ifdef IGNORE_ALL_KEYS_WHEN_RUNNING
  if (currentRpm == 0) {
#endif   // IGNORE_ALL_KEYS_WHEN_RUNNING

    // these should only work when the power is on
    if (this->core->isPowerOn()) {
      // these keys can be operated when the machine is running
      if (keys.bit.UP) {
        core->setFeed(feedTable->next());
      }
      if (keys.bit.DOWN) {
        core->setFeed(feedTable->previous());
      }
    } else {
      // if power off then allow brightness to be changed.
      if (keys.bit.UP)
        controlPanel->incBrightness();
      if (keys.bit.DOWN)
        controlPanel->decBrightness();
    }

#ifdef IGNORE_ALL_KEYS_WHEN_RUNNING
  }
#endif   // IGNORE_ALL_KEYS_WHEN_RUNNING

  // Check for limit Sw and PowerOn, then print messages
  if (core->getLimitSw() && this->core->isPowerOn()) {
    // Check the current Status between Thread or Feed mode, the behavior
    // of the limit switch is sligthly different
    if (this->thread) {
      // Check first if there is a new state
      if (core->getLimitSwState() != limitSwState) {
        limitSwState = core->getLimitSwState();
        // In Thread mode the movement is stopped and it waits until the spindle it's also stopped
        // or reversed, so print the Stop message, and when is stoped reverse message
        switch (limitSwState) {
          case 0:
            // Clear msg
            clearMessage();
          case 1:
            // Limit Switch Reached, print Stop messaje for Stop the Spindle
            setMessage(&LIMIT_SW);
            break;
          case 2:
            // Limit Switch Reached, print Stop messaje for Stopt the Spindle
            setMessage(&STOP);
            break;

          case 3:
            // Set mensage to indicate reverse
            setMessage(&REVERSE);
            break;

          case 4:
            // Set mensage to indicate reverse
            setMessage(&REVERSE);
            break;
        }
      }
    }
    // In Feed mode the Limit Switch stops the movement and only waits until the input is
    // released
    else if (limitSwState == 0) {
      // Set mensage to Limit Switch operation
      setMessage(&LIMIT_SW);
      // Do this just one time
      limitSwState = 1;
    }
  } else if (limitSwState != 0) {
    // Clear display message
    clearMessage();
    // Clear this for next time
    limitSwState = 0;
  }
}

// 'set' menu states, note spacing to allow for 'sub-states'.
enum menuStates {
  kResetPos         = 0,
  kCustomThread     = 0x10,
  kThreadToShoulder = 0x20,
  kShowPosition     = 0x30,
  kQuitMenu         = 0x100
};

// menu loop code

void UserInterface::beginMenu(Uint16 currentRpm) {
  this->isInMenu = true;
  // Reset position menu is only available when is stopped and it's showing the angle
  if ((currentRpm == 0) && (this->showAngle))
    this->menuState = kResetPos;
  // If not start with the custom thread
  else
    this->menuState = kCustomThread;
  this->menuSubState = 0;
  this->numStarts    = 1;
}

void UserInterface::cycleOptions(Uint16 next, Uint16 prev) {
  // if set then start loop
  if (keys.bit.SET)
    this->menuState++;

  // if down then next option on menu
  else if (keys.bit.DOWN) {
    this->menuState    = next;
    this->menuSubState = 0;
  }

  // if up then prev option on menu
  else if (keys.bit.UP) {
    this->menuState    = prev;
    this->menuSubState = 0;
  }

  // if timeout then end menu
  else if (this->messageTime == 0 && !limitSwState)
    this->menuState = kQuitMenu;
}

void UserInterface::menuLoop(Uint16 currentRpm) {
  // check for exit from custom thread
  if (keys.bit.POWER) {
    // Reset everything just in case
    controlPanel->showCurMode(BLANK, BLANK);
    feedTableFactory->flashCustomOff();
    core->setFeed(loadFeedTable());
    clearMessage();
    core->beginThreadToShoulder(false);
    this->isInMenu  = false;
    this->menuState = kQuitMenu;
  }

  switch (this->menuState) {
    // Reset position
    case kResetPos:   // init
      setMessage(&RESET_POS);
      this->menuState++;
      break;
    case kResetPos + 1:
      // wait for keypress, either select this option, move to new or timeout
      cycleOptions(kShowPosition, kCustomThread);   // link any other menu options here
      break;
    case kResetPos + 2:                             // run loop
      // Reset Zero angle
      encoder->setZeroAngle();
      clearMessage();
      this->menuState = kQuitMenu;
      break;

    // custom threads
    case kCustomThread:   // init
      setMessage(&CUSTOM_THREAD);
      this->menuState++;
      break;
    case kCustomThread + 1:
      // wait for keypress, either select this option, move to new or timeout
      if ((currentRpm == 0) && (this->showAngle))
        cycleOptions(kResetPos, kThreadToShoulder);       // link any other menu options here
      else
        cycleOptions(kShowPosition, kThreadToShoulder);   // link any other menu options here
      break;
    case kCustomThread + 2:                               // run loop
      controlPanel->showCurMode(LETTER_R | POINT, LETTER_E | POINT);
      customThreadLoop(currentRpm);
      break;

    // Thread to shoulder
    case kThreadToShoulder:   // init
      setMessage(&THREAD_TO_SHOULDER);
      this->menuState++;
      break;
    case kThreadToShoulder + 1:
      // wait for keypress, either select this option, move to new or timeout
      this->thread = true;
      core->setFeed(loadFeedTable());
      core->setThread(this->thread);
      cycleOptions(kCustomThread, kShowPosition);   // link any other menu options here
      break;
    case kThreadToShoulder + 2:                     // run loop
      threadToShoulderLoop(currentRpm);
      break;

    // Spindle position / RPM
    case kShowPosition:   // init
      setMessage(showAngle ? &RPM : &POSITION);
      this->menuState++;
      break;
    case kShowPosition + 1:
      // wait for keypress, either select this option, move to new or timeout
      if ((currentRpm == 0) && (this->showAngle))
        cycleOptions(kThreadToShoulder, kResetPos);       // link any other menu options here
      else
        cycleOptions(kThreadToShoulder, kCustomThread);   // link any other menu options here
      break;
    case kShowPosition + 2:                               // run loop
      showAngle       = !showAngle;
      this->menuState = kQuitMenu;
      break;

    // exit setup
    case kQuitMenu:
      clearMessage();
      this->isInMenu = false;
      break;
  }
}

void UserInterface::customThreadLoop(Uint16 currentRpm) {
  // check for exit from custom thread
  if (keys.bit.POWER)
    this->menuSubState = 4;

  switch (this->menuSubState) {
    // initialise
    case 0:
      clearMessage();
      feedTableFactory->useCustomPitch = true;
      this->menuSubState               = 1;
      break;

      // edit custom pitch 100's
    case 1:
      feedTableFactory->flashCustomDigit(1);

      if (keys.bit.UP)
        feedTableFactory->incCustomDigit(1);
      if (keys.bit.DOWN)
        feedTableFactory->decCustomDigit(1);

      if (keys.bit.SET)
        this->menuSubState = 2;
      break;

      // edit custom pitch 10's
    case 2:
      feedTableFactory->flashCustomDigit(2);

      if (keys.bit.UP)
        feedTableFactory->incCustomDigit(2);
      if (keys.bit.DOWN)
        feedTableFactory->decCustomDigit(2);

      if (keys.bit.SET)
        this->menuSubState = 3;
      break;

      // edit custom pitch 1's
    case 3:
      feedTableFactory->flashCustomDigit(3);

      if (keys.bit.UP)
        feedTableFactory->incCustomDigit(3);
      if (keys.bit.DOWN)
        feedTableFactory->decCustomDigit(3);

      if (keys.bit.SET)
        this->menuSubState = 4;
      break;

    case 4:
      controlPanel->showCurMode(BLANK, BLANK);
      feedTableFactory->flashCustomOff();
      core->setFeed(loadFeedTable());

      this->isInMenu = false;
      break;
  }
}

void UserInterface::threadToShoulderLoop(Uint16 currentRpm) {
  // check for exit from thread to shoulder
  if (keys.bit.POWER)
    this->menuSubState = 10;

  switch (this->menuSubState) {
    // number of thread 'starts'
    case 0:
      if (keys.bit.UP && this->numStarts < 9)
        this->numStarts++;
      else if (keys.bit.DOWN && this->numStarts > 1)
        this->numStarts--;
      else if (keys.bit.UP && this->numStarts == 9)
        this->numStarts = 1;
      else if (keys.bit.DOWN && this->numStarts == 1)
        this->numStarts = 9;

      MULTI.message[7] = this->feedTableFactory->valueToDigit(this->numStarts);
      setMessage(&MULTI);

      if (keys.bit.SET) {
        this->menuSubState = 1;
        this->currentStart = 0;
      }
      break;

      // prompt user to 'go to shoulder'
    case 1:
      setMessage(&GO_SHOULDER);
      if (keys.bit.SET) {
        core->setShoulder();
        this->menuSubState = 2;
      }
      break;

      // prompt user to 'go to start'
    case 2:
      setMessage(&GO_START);
      if (keys.bit.SET) {
        core->setStart();
        core->beginThreadToShoulder(true);
        setMessage(&BEGIN);
        this->menuSubState = 3;
      }
      break;

      // begin threading
    case 3:
      BEGIN.message[6] = this->feedTableFactory->valueToDigit(this->currentStart + 1) | POINT;
      BEGIN.message[7] = this->feedTableFactory->valueToDigit(this->numStarts);
      setMessage(&BEGIN);
      if (currentRpm != 0)
        this->menuSubState = 4;
      break;

      // wait until we reach the shoulder
    case 4:
      if (core->isAtShoulder())
        this->menuSubState = 5;
      break;

      // at shoulder, wait for user to stop machine
    case 5:
      if (currentRpm == 0) {
        core->resetToShoulder();

        // index start
        this->currentStart++;
        if (this->currentStart >= this->numStarts)
          this->currentStart = 0;

        if (this->numStarts > 1)
          core->setStartOffset(1.0f / (float) this->numStarts);

        this->menuSubState = 6;
      } else
        setMessage(&STOP);
      break;

      // stopped wait for retract
    case 6:
      setMessage(&RETRACT);
      if (keys.bit.UP || keys.bit.SET) {
        core->moveToStart();
        this->menuSubState = 7;
      }

      if (core->isAtStart())
        this->menuSubState = 3;
      break;

      // automatically move to start and repeat
    case 7:
      setMessage(&WAIT);
      if (core->isAtStart())
        this->menuSubState = 3;
      break;

      // finished so quit
    case 10:
      clearMessage();
      core->beginThreadToShoulder(false);
      this->isInMenu = false;
      break;
  }
}
