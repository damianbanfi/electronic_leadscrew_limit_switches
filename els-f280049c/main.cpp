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

#include "Configuration.h"
#include "ControlPanel.h"
#include "EEPROM.h"
#include "Encoder.h"
#include "F28x_Project.h"
#include "SanityCheck.h"
#include "StepperDrive.h"

#include "Core.h"
#include "Debug.h"
#include "UserInterface.h"

// Interrupt Function Prototypes
__interrupt void cpu_timer0_isr(void);
__interrupt void xint1_isr(void);

// Debug harness
Debug debug;

// Feed table factory
FeedTableFactory feedTableFactory;

// Common SPI Bus driver
SPIBus spiBus;

// Control Panel driver
ControlPanel controlPanel(&spiBus);

// EEPROM driver
EEPROM eeprom(&spiBus);

// Encoder driver
Encoder encoder;

// Stepper driver
StepperDrive stepperDrive;

// Core engine
Core core(&encoder, &stepperDrive);

// User interface
UserInterface userInterface(&controlPanel, &core, &feedTableFactory, &encoder);

void main(void) {
#ifdef _FLASH
  // Copy time critical code and Flash setup code to RAM
  // The RamfuncsLoadStart, RamfuncsLoadEnd, and RamfuncsRunStart
  // symbols are created by the linker. Refer to the linker files.
  memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t) &RamfuncsLoadSize);

  // Initialize the flash instruction fetch pipeline
  // This configures the MCU to pre-fetch instructions from flash.
  InitFlash();
#endif

  // Initialize System Control:
  // PLL, WatchDog, enable Peripheral Clocks
  InitSysCtrl();

  // Disable CPU interrupts
  DINT;

  // Initialize the PIE control registers to their default state.
  InitPieCtrl();

  // Disable CPU interrupts and clear all CPU interrupt flags
  IER = 0x0000;
  IFR = 0x0000;

  // Initialize the PIE vector table with pointers to shell Interrupt
  // Service Routines (ISR) to help with debugging.
  InitPieVectTable();

  // Set up the CPU0 timer ISR
  EALLOW;
  // Configure interrupt functions pointers
  PieVectTable.TIMER0_INT = &cpu_timer0_isr;
  PieVectTable.XINT1_INT  = &xint1_isr;
  EDIS;

  // initialize the CPU timer
  InitCpuTimers();   // For this example, only initialize the Cpu Timers
  ConfigCpuTimer(&CpuTimer0, CPU_CLOCK_MHZ, STEPPER_CYCLE_US);

  // Use write-only instruction to set TSS bit = 0
  CpuTimer0Regs.TCR.all = 0x4001;

  // Initialize peripherals and pins
  debug.initHardware();
  spiBus.initHardware();
  controlPanel.initHardware();
  eeprom.initHardware();
  stepperDrive.initHardware();
  encoder.initHardware();

  // Configure Led 5 for debug
  EALLOW;
  GPIO_SetupPinOptions(LED_5, GPIO_OUTPUT, GPIO_OPENDRAIN);
  EDIS;

#ifdef LIMIT_SW_FUNCTIONALITY
  // Configure Limit-Switch input for interrupt on GPIO10, for XINT1
  GPIO_SetupPinOptions(LIMIT_SW_GPIO, GPIO_INPUT, GPIO_PULLUP | GPIO_SYNC);
  GPIO_SetupXINT1Gpio(LIMIT_SW_GPIO);
  // Configure XINT1
  XintRegs.XINT1CR.bit.POLARITY = 0;   // Falling edge interrupt
  // Enable XINT1in the PIE: Group 1 interrupt 4
  // Enable INT1 which is connected to WAKEINT:
  PieCtrlRegs.PIECTRL.bit.ENPIE = 1;   // Enable the PIE block
  PieCtrlRegs.PIEIER1.bit.INTx4 = 1;   // Enable PIE Group 1 INT4
#endif
  // Enable XINT1
  XintRegs.XINT1CR.bit.ENABLE = 1;
  // Enable TINT0 in the PIE: Group 1 interrupt 7
  PieCtrlRegs.PIEIER1.bit.INTx7 = 1;

  // Enable CPU INT1 which is connected to CPU-Timer 0
  IER |= M_INT1;

  // Enable global Interrupts and higher priority real-time debug events
  EINT;
  ERTM;

  // User interface loop
  for (;;) {
    // mark beginning of loop for debugging
    debug.begin2();

    // check for step backlog and panic the system if it occurs
    if (stepperDrive.checkStepBacklog()) {
      userInterface.panicStepBacklog();
    }

    // service the user interface
    userInterface.loop();

    // mark end of loop for debugging
    debug.end2();

    // delay
    DELAY_US(1000000 / UI_REFRESH_RATE_HZ);
  }
}

// CPU Timer 0 ISR
__interrupt void cpu_timer0_isr(void) {
  CpuTimer0.InterruptCount++;

  // flag entrance to ISR for timing
  debug.begin1();

  // service the Core engine ISR, which in turn services the StepperDrive ISR
  core.ISR();

  // flag exit from ISR for timing
  debug.end1();

  //
  // Acknowledge this interrupt to receive more interrupts from group 1
  //
  PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}

// XINT1 ISR
__interrupt void xint1_isr(void) {
  // Assert the flag to indicate the limit Switch has been reached
  stepperDrive.limitSwReached();
  // Acknowledge this interrupt to receive more interrupts from group 1
  PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
