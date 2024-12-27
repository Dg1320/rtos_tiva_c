// Shell functions
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
#include "shell.h"
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "kernel.h"
#include "mm.h"

// DONE: Add header files here for your strings functions, ...
#define RED_LED (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 1*4)))
// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by mutex
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// DONE: add processing for the shell commands through the UART here
void shell(void)
{
    USER_DATA data;
        char* on_off;
        char* pid_name;
        char* help;

        uint32_t pidcheck;
        uint8_t newPriority;
        bool select;
        bool priority = true;
        bool turnOnPreemption = true;

        while(1)
        {

            if(kbhitUart0())                                // key press detected from user
            {
                getsUart0(&data);                           // get the characters from user
                data.fieldCount = 0;
                parseFields(&data);                         // Parse fields

                                                            // have to SVC these calls to access global variables

                if (isCommand(&data, "ps", 1))                      // Command evaluation
                {
                   ps();
                }

                else if (isCommand(&data, "ipcs", 1))
                {
                   ipcs();
                }

                else if (isCommand(&data, "kill", 2))
                {
                    pidcheck = getFieldInteger(&data, 1);

                   kill(pidcheck);
                }

                else if (isCommand(&data, "pi", 2))
                {

                    on_off = getFieldString(&data, 1);

                   if(compare_2words(on_off, "on"))
                   {
                       select = true;
                       pi(select);
                   }

                   else if(compare_2words(on_off, "off"))
                   {
                       select = false;
                       pi(select);
                   }
                   else putsUart0("Invalid command\n");

                }

                else if (isCommand(&data, "preempt", 2))
                {

                    on_off = getFieldString(&data, 1);

                    if(compare_2words(on_off, "on"))
                    {
                        turnOnPreemption = true;
                        preempt(turnOnPreemption);
                    }

                    else if(compare_2words(on_off, "off"))
                    {
                        turnOnPreemption = false;
                        preempt(turnOnPreemption);
                    }
                    else putsUart0("Invalid command\n");

                }

                else if (isCommand(&data, "sched", 2))
                {

                    on_off = getFieldString(&data, 1);

                    if(compare_2words(on_off, "prio"))
                    {
                        priority = true;
                        sched(priority);
                    }

                    else if(compare_2words(on_off, "rr"))
                    {
                        priority = false;
                        sched(priority);
                    }
                    else putsUart0("Invalid command\n");

                }

                else if (isCommand(&data, "pidof", 2))
                {

                    pid_name = getFieldString(&data, 1);

                    pidof(pid_name);

                }

                else if (isCommand(&data, "pkill", 2))
                {

                    pid_name = getFieldString(&data, 1);

                    pkill(pid_name);

                }

                else if (isCommand(&data, "reboot", 1))
                {
                        reboot();
                }
                else if (isCommand(&data, "rs", 2))
                {
                    pidcheck = getFieldInteger(&data, 1);

                    restartTaskByPID(pidcheck);
                }
                else if (isCommand(&data, "rn", 2))
                {
                    pid_name = getFieldString(&data, 1);

                    restartTaskByName(pid_name);
                }
                else if (isCommand(&data, "meminfo", 1))
                {

                    meminfo();
                }
                else if (isCommand(&data, "cp", 1))
                {
                    pidcheck = getFieldInteger(&data, 1);
                    newPriority = getFieldInteger(&data, 2);
                    changePriority(pidcheck,newPriority);
                }
                else if (isCommand(&data, "Idle", 1))
                {
                    restartTaskByName("Idle");
                }
                else if (isCommand(&data, "LengthyFn", 1))
                {
                    restartTaskByName("LengthyFn");

                }
                else if (isCommand(&data, "Flash4Hz", 1))
                {
                    restartTaskByName("Flash4Hz");

                }
                else if (isCommand(&data, "OneShot", 1))
                {
                    restartTaskByName("OneShot");

                }
                else if (isCommand(&data, "ReadKeys", 1))
                {
                    restartTaskByName("ReadKeys");

                }
                else if (isCommand(&data, "Debounce", 1))
                {
                    restartTaskByName("Debounce");

                }
                else if (isCommand(&data, "Important", 1))
                {
                    restartTaskByName("Important");

                }
                else if (isCommand(&data, "Uncoop", 1))
                {
                    restartTaskByName("Uncoop");

                }
                else if (isCommand(&data, "Errant", 1))
                {
                    restartTaskByName("Errant");

                }
                else if (isCommand(&data, "rtos", 1))
                {
                    help = getFieldString(&data, 1);
                    if(compare_2words(help, "-h") || compare_2words(help, "--help"))
                    {
                        putsUart0("----------------------------------------------------------\n");

                        putsUart0("Operations of RTOS shell:\n\n");
                        putsUart0("ps        :                       print status of each process\n\n");
                        putsUart0("meminfo   :                       print memory of each process\n\n");

                        putsUart0("PID       : ");
                        putsUart0("pidof {name}          shows the PID of the {name}\n");
                        putsUart0("          : pkill {name}          kill the process by name\n");
                        putsUart0("          : kill  {pid}           kill the process by PID\n");
                        putsUart0("          : rn    {name}          restart the process by PID\n");
                        putsUart0("          : rs    {pid}           restart the process by PID\n\n");
                        putsUart0("          : cp    {pid}{VALUE}    change priority of process \n");
                        putsUart0("          :                       (0-15), 0 = Highest Priority\n\n");

                        putsUart0("Scheduler : ");
                        putsUart0("sched [prio]          turn on priority scheduler\n");
                        putsUart0("          : sched [rr]            turn on round robin scheduler\n\n");

                        putsUart0("Preemption: ");
                        putsUart0("preempt [on]          turn on preemption\n");
                        putsUart0("          : preempt [off]         turn off preemption\n\n");


                        putsUart0("reboot    :                       reboot the rtos\n");

                        putsUart0("----------------------------------------------------------\n");

                    }
                }

            }

            yield();

         }
}

void meminfo(void)
{
    char print[10];
    int i;
    pidInfo *myMemoryInfo  = (pidInfo* )0x20006000;                  // start at the base of memory to avoid overwriting whats on the stack
    __asm__ (" MOV R12, R0");                                       // move the pointer to my struct to R12 to avoid being tampered with

    __asm__ (" SVC #18");              // process info SVC

     putsUart0("\nName| Stack Base: | Heap Base:   \n");
     putsUart0("----------------------------------------------------------\n\n");


     for( i = 0; i < myMemoryInfo[0].tatalTaskCount; i ++)                                    // cycle through only the amount of PID's available
     {
         putsUart0(myMemoryInfo[i].name);
         putsUart0(" | ");

         putsUart0("Stack: ");
         num_to_HEX(myMemoryInfo[i].stackBaseAdress,print);
         putsUart0(print);  putsUart0(": ");

         num_to_HEX(myMemoryInfo[i].stackMemoryAmount,print);
         putsUart0(print);  putsUart0(" Bytes");
         putsUart0(" | ");

         putsUart0("Heap: ");
         if(myMemoryInfo[i].heapBaseAdress == 0) putsUart0("NONE \n\n");
         else
         {
         num_to_HEX(myMemoryInfo[i].heapBaseAdress,print);
         putsUart0(print);
         putsUart0(": ");

         num_to_HEX(myMemoryInfo[i].heapMemoryAmount,print);
         putsUart0(print);  putsUart0(" Bytes");
         putsUart0("\n\n");
         }
     }
     putsUart0("----------------------------------------------------------\n");

}

void reboot(void)                          // reboots the microcontroller
{
    // reboot() page 164 of the data sheet
    __asm__ (" SVC #15");              // reboot SVC
}

void printMyStatus(uint8_t status,uint8_t mutex, uint8_t semaphore, uint8_t ticks)
{
    int index;
    char print[10];
    switch (status)
        {
            case STATE_INVALID:// no task
                        putsUart0("Invalid");
                                break;

            case STATE_STOPPED:// stopped, all memory freed
                        putsUart0("Stopped");
                                break;

            case STATE_READY:// has run, can resume at any time
                        putsUart0("Ready");
                                break;

            case STATE_DELAYED:// has run, but now awaiting timer
                        putsUart0("Delayed => ");
                        index = int_to_ascii(ticks, print);
                        putsUart0(&print[index]);                             // start at this index to remove leading zeroes
                        putsUart0("ms remaining");
                                break;

            case STATE_BLOCKED_MUTEX:// has run, but now blocked by mutex
                        putsUart0("Blocked by mutex => ");
                        if(resource == mutex) putsUart0("Resource");
                                break;

            case STATE_BLOCKED_SEMAPHORE:// has run, but now blocked by semaphore
                        putsUart0("Blocked by semaphore => ");
                        if(keyPressed == semaphore) putsUart0("keyPressed");
                        else if(keyReleased == semaphore) putsUart0("keyReleased");
                        else if(flashReq == semaphore) putsUart0("flashReq");
                                break;

        }

}

void print_CPU_usage(uint8_t printIndex, const char myPercentage[] )
{
    int i;
    char leftSideOfDecimal [2];
    char rightSideOfDecimal [2];


    switch (printIndex)
            {
                case 6:// XX.XX

                         for(i = 0; i<2; i++)                    // get the putsUart to only print 2 values
                         {
                             leftSideOfDecimal[i] = myPercentage[i];
                             rightSideOfDecimal[i]= myPercentage[2+i];

                         }
                         putsUart0(leftSideOfDecimal); putsUart0("."); putsUart0(rightSideOfDecimal); putsUart0("%");
                         putsUart0(": ");
                                    break;

                case 7:// 0X.XX
                        for(i = 0; i<2; i++)                    // get the putsUart to only print 2 values
                         {
                             leftSideOfDecimal[i] = myPercentage[0];
                             rightSideOfDecimal[i]= myPercentage[2+i];

                         }
                         putsUart0(leftSideOfDecimal); putsUart0("."); putsUart0(rightSideOfDecimal); putsUart0("%");
                         putsUart0(": ");
                     break;

                case 8:// 0.XX
                        for(i = 0; i<2; i++)                    // get the putsUart to only print 2 values
                         {
                             rightSideOfDecimal[i]= myPercentage[i];
                         }
                        putsUart0("0"); putsUart0("."); putsUart0(rightSideOfDecimal); putsUart0("%");
                        putsUart0(": ");
                                    break;

                case 9:// 00.0X
                        rightSideOfDecimal[0]= myPercentage[0];
                        putsUart0("0"); putsUart0(".");putsUart0("0"); putsUart0(rightSideOfDecimal); putsUart0("%");
                        putsUart0(": ");

                                    break;


            }


}

uint32_t getADD()
{
    __asm__ (" SVC #21");              // process info SVC
    return;
}

void ps(void)                              // displays the process (thread) status
{
    char print[10];
    uint8_t i,index,mutex,semaphore;

    uint32_t shellBaseAdd = getADD();

    pidInfo *printMyPidInfo  = (pidInfo* )shellBaseAdd;     // start at the base of memory to avoid overwriting whats on the stack

    __asm__ (" MOV R12, R0");              // move the pointer to my struct to R12 to avoid being tampered with

    __asm__ (" SVC #14");              // process info SVC

    putsUart0("\nName| %CPU Time: Priority: State  \n");
    putsUart0("----------------------------------------------------------\n\n");


    for( i = 0; i < printMyPidInfo[0].tatalTaskCount; i ++)                                    // cycle through only the amount of PID's available
    {
        putsUart0(printMyPidInfo[i].name);
        putsUart0("| ");

        index = int_to_ascii(printMyPidInfo[i].cpuUsage, print);
        print_CPU_usage(index ,&print[index]);

        index = int_to_ascii(printMyPidInfo[i].currentPriority, print);
        putsUart0(&print[index]);
        putsUart0(": ");
        printMyStatus(printMyPidInfo[i].state,printMyPidInfo[i].mutex, printMyPidInfo[i].semaphore, printMyPidInfo[i].ticks);
        putsUart0("\n\n");
    }
    putsUart0("----------------------------------------------------------\n");
}


void printSemaphoreName(uint8_t semaphoreRepresentation)
{
        switch (semaphoreRepresentation)
            {
                case 0:
                            putsUart0("keyPressed");
                                    break;

                case 1:
                            putsUart0("keyReleased");
                                    break;

                case 2:
                            putsUart0("flashReq");
                                    break;
            }

}

void printMutexName(uint8_t mutexRepresentation)
{
    switch (mutexRepresentation)
        {
            case 0:
                        putsUart0("resource");
                                break;
        }
}
void ipcs(void)                            // display inter-process (thread) communication status
{
    int i,j,index;
    char print[10];

    uint32_t shellBaseAdd = getADD();

    ipcsInfo *printMyIpcsInfo  = (ipcsInfo* )shellBaseAdd;     // start at the base of memory to avoid overwriting whats on the stack

    __asm__ (" MOV R12, R0");              // move the pointer to my struct to R12 to avoid being tampered with

    __asm__ (" SVC #20");              // ipcs svc called

    putsUart0("\nMutex and Semaphore Info\n");
    putsUart0("----------------------------------------------------------\n");

    for(i = 0; i < MAX_MUTEXES ; i ++)
   {
       if(printMyIpcsInfo->lock[i] == true)
       {
           putsUart0("\nMutex: ");
           printMutexName(i);
           putsUart0(" => Locked By: ");
           index = int_to_ascii(printMyIpcsInfo->mutexLockedBy[i], print);
           putsUart0(&print[index]);
       }

       if(printMyIpcsInfo->mutexQueSize[i] > 0)
       {    putsUart0(" => Task Waiting: ");

           for(j = 0; j < printMyIpcsInfo->mutexQueSize[i] ; j ++)
           {
               index = int_to_ascii(printMyIpcsInfo->mutexProcessQue[i][j], print);
               putsUart0(&print[index]);
           }
       }

       putsUart0("\n\n");

   }

    for(i = 0; i < MAX_SEMAPHORES ; i ++)
    {
            putsUart0("Semaphore: ");
            printSemaphoreName(i);
            putsUart0(" => Count: ");
            index = int_to_ascii(printMyIpcsInfo->semaphoreCount[i], print);
            putsUart0(&print[index]);

        if(printMyIpcsInfo->semaphoreQueSize[i] > 0)
        {    putsUart0(" => Task Waiting: ");

            for(j = 0; j < printMyIpcsInfo->semaphoreQueSize[i] ; j ++)
            {
                index = int_to_ascii(printMyIpcsInfo->semaphoreProcessQue[i][j], print);
                putsUart0(&print[index]);
            }
        }

        putsUart0("\n");


    }
    putsUart0("----------------------------------------------------------\n");

}



void kill(uint32_t pid)                    // kills process (thread) with matching PID
{
    __asm__ (" SVC #12");              // Kill PID SVC
}

void pkill(const char name[])              // kills the thread based on process name
{
    __asm__ (" SVC #13");              // finding PID name SVC
}

void pi(bool on)                           // turns priority inheritance on/off
{
    if(on == true)
    {
        putsUart0("pi on!\n");
    }
    else
    {
        putsUart0("pi off!\n");
    }


}

void preempt(bool on)                      // turns preemption on/off
{
    __asm__ (" SVC #10");              // preemption SVC
    if(on == true)
     {
         putsUart0("Preemption On\n");
     }
     else
     {
         putsUart0("Preemption Off\n");
     }

}

void sched(bool priority)                   // selects priority or round-robin Scheduling
{
    __asm__ (" SVC #9");              // schedule SVC

     if(priority == true)
     {
         putsUart0("Schedule: Priority\n");
     }
     else
     {
         putsUart0("Schedule: Round Robin\n");
     }

}

void pidof(const char name[])                    // display PID of process (thread)
{
    uint32_t pidNumber;
    char pidPrint[10];
    uint8_t index;

    __asm__ (" SVC #11");              // finding PID name SVC
    pidNumber = returnR0();
    if(!pidNumber)
    {
        putsUart0("Task: ");
        putsUart0(name);
        putsUart0(" either unknown or not running\n");
    }
    else
    {
        putsUart0(name);
        putsUart0(" PID: ");
        index = int_to_ascii(pidNumber, pidPrint);
        putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
        putsUart0("\n");
    }


}
void restartTaskByName(const char name[])
{

    __asm__ (" SVC #22");              // finding PID name SVC

}

void restartTaskByPID(uint32_t pid)
{

    __asm__ (" SVC #8");              // finding PID name SVC

}

void changePriority(uint32_t pid, uint8_t priority)
{
    __asm__ (" MOV R3, R0");                        // move variable to avoid being overwritten
    __asm__ (" MOV R12, R1");                        // move variable to avoid being overwritten

    __asm__ (" SVC #19");                            // supervisor call changing thread priority

}
