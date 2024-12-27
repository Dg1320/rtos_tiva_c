// Memory Protection Unit Declarations
// DeZean Gardner

#ifndef MPU_CONTROL_H_
#define MPU_CONTROL_H_

#include <stdint.h>
#include "kernel.h"
extern void gobacktoPSP();                             // exception return.. unprededictable if not used even if only in SVC and not PendSV

extern void setPSP(uint32_t *address);                 // set process stack pointer
extern void setASP();                                  // set active stack pointer
extern void userMode();                                // set privilege bit to USER
extern void kernelMode();                              // set privilege bit to KERNEL
extern void appearAsRanTask(uint32_t *address);        // make all tasks to appear as they Ran before
extern void runFirstTask(uint32_t *address);           // start up the First task in scheduler
extern void saveR4toR11(uint32_t *address);            // start up the First task in scheduler
extern void taskSwitch(uint32_t *address);             // start up the First task in scheduler
extern uint8_t getSVCallNumber();

extern uint32_t *getPSP();                             // get process stack pointer value
extern uint32_t *getMSP();                             // get main stack pointer value

extern uint32_t getXPSR();                             // ALL THE STATUS REGISTERS BELOW ARE INCLUDED HERE
extern uint32_t getAPSR();                             // get Application Program Status Register
extern uint32_t getEPSR();                             // get Execution Program Status Register
extern uint32_t getIPSR();                             // get Interrupt Program Status Register

extern uint32_t returnR0();                             // just return whatever is in R0 at the time
extern uint32_t returnR3();                             // just return whatever is in R3 at the time
extern uint32_t returnR12();                            // just return whatever is in R12at the time
extern pidInfo returnPIDinfoAddress();                  // just return whatever pid struct pointer

extern uint32_t *appearAsRanTaskAgain(uint32_t *address); // this time return the address of where to begin again

#endif
