// Fault ISR's
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
#include "tm4c123gh6pm.h"
#include "faults.h"
#include "uart0.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// DONE: If these were written in assembly
//           omit this file and add a faults.s file

// DONE: code this function
void mpuFaultIsr(void)
{
    char register_display [9];
       uint32_t *temp_address;
       uint32_t *REG_address;
       uint32_t *PSP;
       uint32_t temp_value = 0;
       // PSP = Process Stack Pointer
       // MSP = Main Stack Pointer
       // mfault flag
       //              IERR    = Instruction Access Violation
       //              DERR    = Data Access Violation
       //              MUSTKE  = Unstack Access Violation
       //              MSTKE   = Stack Access Violation
       //              MLSPERR = Memory Management Fault on Floating-Point Lazy State Preservation
       //              MMARV   = Memory Management Fault Address Register Valid
       // xPSR = Program status register
       // PC = Program Counter
       // LR = Link Register
       // R0-R3
       // R12
       putsUart0("------------------------------------------------");
       putsUart0("\nMPU fault in process!\n");
       //////////////////////////////
       temp_address = getMSP();
       putsUart0("MSP   : ");
       num_to_HEX((uint32_t )temp_address,register_display);
       putsUart0(register_display);
       putsUart0("\n");
       //////////////////////////////
       PSP = getPSP();
       putsUart0("PSP   : ");
       num_to_HEX((uint32_t )PSP,register_display);            // casted to print value
       putsUart0(register_display);
       putsUart0("\n");
       //////////////////////////////
       temp_value = NVIC_FAULT_STAT_R & 0x000000FF;
       putsUart0("MFAULT: ");
       num_to_HEX(temp_value, register_display);
       putsUart0(register_display);
       putsUart0("\n");
       //////////////////////////////
       REG_address = PSP + 6;
       putsUart0("\nInstruction @ ");
       num_to_HEX((uint32_t )*REG_address,register_display);            // casted to print value
       putsUart0(register_display);
       putsUart0(" tried accessing ");
       temp_value = NVIC_MM_ADDR_R;                                    // see datasheet page 184
       num_to_HEX((uint32_t )temp_value,register_display);            // casted to print value
       putsUart0(register_display);
       putsUart0("\n");
       //////////////////////////////

       putsUart0("\nPROCESS STACK DUMP:\n");

       //////////////////////////////
       temp_value = getXPSR();

       putsUart0("xPSR  : ");
       num_to_HEX(temp_value,register_display);
       putsUart0(register_display);
       putsUart0("\n");
       //////////////////////////////              // page 110 document sheet

       REG_address = PSP + 6;
       putsUart0("PC    : ");
       num_to_HEX((uint32_t )*REG_address,register_display);
       putsUart0(register_display);
       putsUart0("\n");
       //////////////////////////////
       REG_address = PSP + 5;
       putsUart0("LR    : ");
       num_to_HEX((uint32_t )*REG_address,register_display);
       putsUart0(register_display);
       putsUart0("\n");
       //////////////////////////////

       REG_address = PSP + 4;
       putsUart0("R12   : ");
       num_to_HEX((uint32_t )*REG_address,register_display);
       putsUart0(register_display);
       putsUart0("\n");

       REG_address = PSP + 3;
       putsUart0("R3    : ");
       num_to_HEX((uint32_t )*REG_address,register_display);
       putsUart0(register_display);
       putsUart0("\n");

       REG_address = PSP + 2;
       putsUart0("R2    : ");
       num_to_HEX((uint32_t )*REG_address,register_display);
       putsUart0(register_display);
       putsUart0("\n");

       REG_address = PSP + 1;
       putsUart0("R1    : ");
       num_to_HEX((uint32_t )*REG_address,register_display);
       putsUart0(register_display);
       putsUart0("\n");

       putsUart0("R0    : ");
       num_to_HEX((uint32_t )*PSP,register_display);
       putsUart0(register_display);
       putsUart0("\n");
       NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;
       NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
       putsUart0("------------------------------------------------\n");

       stopCurrentTask((uint32_t )PSP);
       NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;
       NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
}

// DONE: code this function
void hardFaultIsr(void)
{

    char register_display [9];
    uint32_t *temp_address;
    uint32_t *PSP;
    uint32_t *REG_address;
    uint32_t temp_value = 0;
    // PSP = Process Stack Pointer
    // MSP = Main Stack Pointer
    // mfault flag
    //              IERR    = Instruction Access Violation
    //              DERR    = Data Access Violation
    //              MUSTKE  = Unstack Access Violation
    //              MSTKE   = Stack Access Violation
    //              MLSPERR = Memory Management Fault on Floating-Point Lazy State Preservation
    //              MMARV   = Memory Management Fault Address Register Valid
    // xPSR = Program status register
    // PC = Program Counter
    // LR = Link Register
    // R0-R3
    // R12
    putsUart0("------------------------------------------------");
    putsUart0("\nHard fault in process!\n");
    //////////////////////////////
    temp_address = getMSP();
    putsUart0("MSP   : ");
    num_to_HEX((uint32_t )temp_address,register_display);
    putsUart0(register_display);
    putsUart0("\n");
    //////////////////////////////
    PSP = getPSP();
    putsUart0("PSP   : ");
    num_to_HEX((uint32_t )PSP,register_display);            // casted to print value
    putsUart0(register_display);
    putsUart0("\n");
    //////////////////////////////
    temp_value = NVIC_FAULT_STAT_R & 0x000000FF;
    putsUart0("MFAULT: ");
    num_to_HEX(temp_value, register_display);
    putsUart0(register_display);
    putsUart0("\n");
    //////////////////////////////

    REG_address = PSP + 6;
    putsUart0("\nInstruction @ ");
    num_to_HEX((uint32_t )*REG_address,register_display);            // casted to print value
    putsUart0(register_display);
    putsUart0(" tried accessing ");
    temp_value = NVIC_MM_ADDR_R;                                    // see datasheet page 184
    num_to_HEX((uint32_t )temp_value,register_display);            // casted to print value
    putsUart0(register_display);
    putsUart0("\n");
    /////////////////////////////

    putsUart0("\nPROCESS STACK DUMP:\n");

    //////////////////////////////
    temp_value = getXPSR();

    putsUart0("xPSR  : ");
    num_to_HEX(temp_value,register_display);
    putsUart0(register_display);
    putsUart0("\n");
    //////////////////////////////              // page 110 document sheet

    REG_address = PSP + 6;
    putsUart0("PC    : ");
    num_to_HEX((uint32_t )*REG_address,register_display);
    putsUart0(register_display);
    putsUart0("\n");
    //////////////////////////////
    REG_address = PSP + 5;
    putsUart0("LR    : ");
    num_to_HEX((uint32_t )*REG_address,register_display);
    putsUart0(register_display);
    putsUart0("\n");
    //////////////////////////////

    REG_address = PSP + 4;
    putsUart0("R12   : ");
    num_to_HEX((uint32_t )*REG_address,register_display);
    putsUart0(register_display);
    putsUart0("\n");

    REG_address = PSP + 3;
    putsUart0("R3    : ");
    num_to_HEX((uint32_t )*REG_address,register_display);
    putsUart0(register_display);
    putsUart0("\n");

    REG_address = PSP + 2;
    putsUart0("R2    : ");
    num_to_HEX((uint32_t )*REG_address,register_display);
    putsUart0(register_display);
    putsUart0("\n");

    REG_address = PSP + 1;
    putsUart0("R1    : ");
    num_to_HEX((uint32_t )*REG_address,register_display);
    putsUart0(register_display);
    putsUart0("\n");

    putsUart0("R0    : ");
    num_to_HEX((uint32_t )*PSP,register_display);
    putsUart0(register_display);
    putsUart0("\n");
    NVIC_SYS_HND_CTRL_R &= ~NVIC_SYS_HND_CTRL_MEMP;
    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
    putsUart0("------------------------------------------------\n");


   while(1);
}

// DONE: code this function
void busFaultIsr(void)
{
     char register_display [9];
     uint32_t* address = NVIC_FAULT_ADDR_R;
   //  uint32_t pid;
     putsUart0("Bus fault in process!\n");
     num_to_HEX((uint32_t )*address,register_display);            // casted to print value
     putsUart0("Bus fault address: ");
     putsUart0(register_display);
     putsUart0("\n");

     address = NVIC_FAULT_STAT_R;
     putsUart0("FAULT FLAGS:");
     num_to_HEX((uint32_t )*address,register_display);            // casted to print value
     putsUart0(register_display);
     putsUart0("\n");
     while(1);
}

// DONE: code this function
void usageFaultIsr(void)
{
    //  uint32_t pid;

//    char register_display [9];
//    uint32_t* address = NVIC_FAULT_STAT_R;
    putsUart0("Usage fault in process!\n");
//    num_to_HEX((uint32_t )*address,register_display);            // casted to print value
//    putsUart0(register_display);
//    putsUart0("\n");

      while(1);
}
