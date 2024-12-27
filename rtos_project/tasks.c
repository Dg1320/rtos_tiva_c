// Tasks
// DeZean Gardner

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "gpio.h"
#include "wait.h"
#include "kernel.h"
#include "tasks.h"

#define BLUE_LED     PORTF,2 // on-board blue LED
#define GREEN_LED    PORTE,0 // off-board red LED
#define ORANGE_LED   PORTA,3 // off-board orange LED
#define YELLOW_LED   PORTA,4 // off-board yellow LED
#define RED_LED      PORTA,2 // off-board green LED

#define BUTTON_0     PORTC,4
#define BUTTON_1     PORTC,5
#define BUTTON_2     PORTC,6
#define BUTTON_3     PORTC,7
#define BUTTON_4     PORTD,6
#define BUTTON_5     PORTD,7

#define PB0_MASK  0b00010000            // PB0    : PC[4]
#define PB1_MASK  0b00100000            // PB1    : PC[5]
#define PB2_MASK  0b01000000            // PB2    : PC[6]
#define PB3_MASK  0b10000000            // PB3    : PC[7]
#define PB4_MASK  0b01000000            // PB4    : PD[6]
#define PB5_MASK  0b10000000            // PB5    : PD[7]
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// DONE: Add initialization for blue, orange, red, green, and yellow LEDs
//           Add initialization for 6 pushbuttons
void initHw(void)
{
    // Enable Clocks
    enablePort(PORTA);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    // Setup pushbuttons
    setPinCommitControl(BUTTON_5);                      // Register now unlocked to be used

    enablePinPullup(BUTTON_0);
    enablePinPullup(BUTTON_1);
    enablePinPullup(BUTTON_2);
    enablePinPullup(BUTTON_3);
    enablePinPullup(BUTTON_4);
    enablePinPullup(BUTTON_5);

    selectPinDigitalInput(BUTTON_0);
    selectPinDigitalInput(BUTTON_1);
    selectPinDigitalInput(BUTTON_2);
    selectPinDigitalInput(BUTTON_3);
    selectPinDigitalInput(BUTTON_4);
    selectPinDigitalInput(BUTTON_5);

    // Setup LEDs
    selectPinPushPullOutput(BLUE_LED);
    selectPinPushPullOutput(GREEN_LED);
    selectPinPushPullOutput(ORANGE_LED);
    selectPinPushPullOutput(YELLOW_LED);
    selectPinPushPullOutput(RED_LED);

    // Power-up flash
    setPinValue(GREEN_LED, 1);
    waitMicrosecond(250000);
    setPinValue(GREEN_LED, 0);
    waitMicrosecond(250000);

}

// DONE: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs(void)
{
       uint8_t myButtonValue = 0;                                               // have to make negative logic because of internal pull up resistor

             if(!getPinValue(BUTTON_0)) myButtonValue = 0b00000001;        // Button 0

        else if(!getPinValue(BUTTON_1)) myButtonValue = 0b00000010;        // Button 1

        else if(!getPinValue(BUTTON_2)) myButtonValue = 0b00000100;        // Button 2

        else if(!getPinValue(BUTTON_3)) myButtonValue = 0b00001000;        // Button 3

        else if(!getPinValue(BUTTON_4)) myButtonValue = 0b00010000;        // Button 4

        else if(!getPinValue(BUTTON_5)) myButtonValue = 0b00100000;        // Button 5

      return myButtonValue;
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle(void)
{
    while(true)
    {
        setPinValue(ORANGE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(ORANGE_LED, 0);
        yield();
    }
}

void idle2(void)                // using this to make sure my tasks are switching properly... Both LEDs should be showing
{

    while(true)
    {
        setPinValue(BLUE_LED, 1);
        waitMicrosecond(1000);
        setPinValue(BLUE_LED, 0);
        yield();
    }
}

void flash4Hz(void)
{
    while(true)
    {
        setPinValue(GREEN_LED, !getPinValue(GREEN_LED));
        sleep(125);

    }


}

void oneshot(void)
{
    while(true)
    {
        wait(flashReq);
        setPinValue(YELLOW_LED, 1);
        sleep(1000);
        setPinValue(YELLOW_LED, 0);
    }
}

void partOfLengthyFn(void)
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn(void)
{
    uint16_t i;
    uint8_t *mem;
    mem = iNeedSomeMemory(5000 * sizeof(uint8_t));
    while(true)
    {
        lock(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
            mem[i] = i % 256;
        }
        setPinValue(RED_LED, !getPinValue(RED_LED));
        unlock(resource);
    }
}

void readKeys(void)
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            setPinValue(YELLOW_LED, !getPinValue(YELLOW_LED));
            setPinValue(RED_LED, 1);
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            setPinValue(RED_LED, 0);
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            stopThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce(void)
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative(void)
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant(void)
{
    uint32_t* p = (uint32_t*)0x20000000;
    while(true)
    {
        while (readPbs() == 32)
        {
            *p = 0;
        }
        yield();
    }
}

    void important(void)
{
    while(true)
    {
        lock(resource);
        setPinValue(BLUE_LED, 1);
        sleep(1000);
        setPinValue(BLUE_LED, 0);
        unlock(resource);
    }
}
