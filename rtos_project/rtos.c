// RTOS Framework - Fall 2024
// J Losh

// Student Name: DeZean Gardner
// DONE: Add your name(s) on this line.
//        Do not include your ID number(s) in the file.

// Please do not change any function name in this code or the thread priorities

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// 6 Pushbuttons and 5 LEDs, UART
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1
// Memory Protection Unit (MPU):
//   Region to control access to flash, peripherals, and bitbanded areas
//   4 or more regions to allow SRAM access (RW or none for task)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include "tm4c123gh6pm.h"
#include "clock.h"
#include "gpio.h"
#include "uart0.h"
#include "wait.h"
#include "mm.h"
#include "kernel.h"
#include "faults.h"
#include "tasks.h"
#include "shell.h"

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
#define PIN_PD3 0b00001000
#define GREEN_LED_MASK 0b00001000
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 3*4)))
void setupCPUTimer(void)
{
        SYSCTL_RCGCWTIMER_R |= SYSCTL_RCGCWTIMER_R3;
        _delay_cycles(3);

       GPIO_PORTD_DIR_R &=  ~PIN_PD3;                  // IR LED
       GPIO_PORTD_DEN_R |= PIN_PD3 ;

       GPIO_PORTD_AFSEL_R |= PIN_PD3;                   // choose alternative function for GPIO
       GPIO_PORTD_PCTL_R &= ~(GPIO_PCTL_PD3_M);        // clear original function
       GPIO_PORTD_PCTL_R |= GPIO_PCTL_PD3_WT3CCP1;     // use as widetimer

       // -------- Set up edge-time mode @ 40MHz for controller  PD[3] for controller

       WTIMER3_CTL_R &= ~TIMER_CTL_TBEN;                         // turn-off counter before reconfiguring
       WTIMER3_CFG_R = 4;                                        // configure as 32-bit counter
                                                                 // TnCMR == 1 and TnMR == 3 then set direction    // PAGE 724
       WTIMER3_TBMR_R |= TIMER_TBMR_TBMR_PERIOD ;                   //  periodic mode / count UP
       WTIMER3_TBMR_R |= TIMER_TBMR_TBCDIR ;

}

void setupPeriodicTimer()
{
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    SYSCTL_RCGCGPIO_R |= SYSCTL_RCGCGPIO_R5;
    _delay_cycles(3);
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD;          // configure for periodic mode (count down)
    TIMER1_TAILR_R = 40000000;                       // set load value to 40e6 for 1 Hz interrupt rate
    TIMER1_IMR_R = TIMER_IMR_TATOIM;                 // turn-on interrupts
    TIMER1_CTL_R |= TIMER_CTL_TAEN;                  // turn-on timer
    NVIC_EN0_R = 1 << (INT_TIMER1A-16);              // turn-on interrupt 37 (TIMER1A)

    GPIO_PORTF_DIR_R |= GREEN_LED_MASK ;             // green LED is an output
    GPIO_PORTF_DEN_R |= GREEN_LED_MASK ;             // enable green LED

}

void flash_greenISR()
{
    GREEN_LED ^= 1;
    TIMER1_ICR_R = TIMER_ICR_TATOCINT;           // clear interrupt flag
}

int main(void)
{

    bool ok;
    // Initialize hardware
    initSystemClockTo40Mhz();
    initHw();
    initUart0();
    setupCPUTimer();
    setupPeriodicTimer();

    allowFlashAccess();
    allowPeripheralAccess();
    setupSramAccess();
    enableBackgroundRegion();
    initRtos();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Initialize mutexes and semaphores
    initMutex(resource);
    initSemaphore(keyPressed, 1);
    initSemaphore(keyReleased, 0);
    initSemaphore(flashReq, 5);

    // Add required idle process at lowest priority
    ok = createThread(idle, "Idle", 15, 512);
//    ok = createThread(idle2, "Idle2",15,512);
//     Add other processes
    ok &= createThread(lengthyFn, "LengthyFn", 12, 1024);
    ok &= createThread(flash4Hz, "Flash4Hz", 8, 512);
    ok &= createThread(oneshot, "OneShot", 4, 1536);
    ok &= createThread(readKeys, "ReadKeys", 12, 1024);
    ok &= createThread(debounce, "Debounce", 12, 1024);
    ok &= createThread(important, "Important", 0, 1024);
    ok &= createThread(uncooperative, "Uncoop", 12, 1024);
    ok &= createThread(errant, "Errant", 12, 512);
    ok &= createThread(shell, "Shell", 12, 4096);

    // DONE: Add code to implement a periodic timer and ISR

    // Start up RTOS
    if (ok)
        startRtos(); // never returns

    else
        while(true);
}
