// Kernel functions
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
#include "mm.h"
#include "kernel.h"
#include "uart0.h"
#include "mpu_control.h"
//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// mutex
typedef struct _mutex
{
    bool lock;
    uint8_t queueSize;
    uint8_t processQueue[MAX_MUTEX_QUEUE_SIZE];
    uint8_t lockedBy;
} mutex;
mutex mutexes[MAX_MUTEXES];

// semaphore
typedef struct _semaphore
{
    uint8_t count;
    uint8_t queueSize;
    uint8_t processQueue[MAX_SEMAPHORE_QUEUE_SIZE];
} semaphore;
semaphore semaphores[MAX_SEMAPHORES];

// task states
#define STATE_INVALID           0 // no task
#define STATE_STOPPED           1 // stopped, all memory freed
#define STATE_READY             2 // has run, can resume at any time
#define STATE_DELAYED           3 // has run, but now awaiting timer
#define STATE_BLOCKED_MUTEX     4 // has run, but now blocked by mutex
#define STATE_BLOCKED_SEMAPHORE 5 // has run, but now blocked by semaphore

// task
uint8_t taskCurrent = 0;          // index of last dispatched task
uint8_t taskCount = 0;            // total number of valid tasks
bool taskPriorityChanged[MAX_TASKS]= {[0 ... MAX_TASKS-1] = false};                                          // keep track if task already changed so we dont create another schedule
                                                                                                             // as a result wasting more time
// control
bool priorityScheduler = true;    // priority (true) or round-robin (false)
bool priorityInheritance = false;  // priority inheritance for mutexes
bool preemption = true;            // preemption (true) or cooperative (false)

uint32_t restart = 100;            // control whether or not we are starting with a clean slate

// CPU time
bool ping = true;
bool pong = false;
uint32_t totalCpuTimeA;
uint32_t totalCpuTimeB;

// tcb
#define NUM_PRIORITIES   16
#define EXTRA_MEMORY_ALLOCATED 7
struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread (add of task fn)
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    uint8_t priority;              // 0=highest
    uint8_t currentPriority;       // 0=highest (needed for pi)
    uint32_t ticks;                // ticks until sleep complete
    uint64_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    uint8_t mutex;                 // index of the mutex in use or blocking the thread
    uint8_t semaphore;             // index of the semaphore that is blocking the thread
    uint32_t mySize;               // keep track of task size for when restarting
    uint32_t cpuUsage;             // CPU usage
    uint64_t timeElaspesA;         // keep track of CPU time
    uint64_t timeElaspesB;         // going back and forth updating the amount of time is being used
} tcb[MAX_TASKS];

uint32_t pid;                      // used for memory management
uint32_t myReturnPrintValue;       // help with controling prints
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex)
{
    bool ok = (mutex < MAX_MUTEXES);
    if (ok)
    {
        mutexes[mutex].lock = false;
        mutexes[mutex].lockedBy = 0;
    }
    return ok;
}

bool initSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// DONE: initialize systick for 1ms system timer
void initRtos(void)
{
    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
    }

    // sysTick initialized for 1ms
    NVIC_ST_CTRL_R = 0;                                                 // disable systick while configuring
    NVIC_ST_RELOAD_R = 40000 - 1;                                       // reload value set to be 1ms based on 40MHz clock
                                                                        // datasheet pg.140 on why subtracting 1
    NVIC_ST_CURRENT_R = 0 ;                                             // any write to current clears count bit which activates the interrupt
    NVIC_ST_CTRL_R = 0x0007;                                            // enable systick x interrupt x systemclock

}

void reinitializePriorites(int8_t myArray[][NUM_PRIORITIES],int rows, int cols)	       // initialize the entire array to no priority
{
    int i,j;

    for (i = 0; i < rows; i++)
    {
        for (j = 0; j < cols; j++)
        {
            myArray[i][j] = -1;
        }
    }
}

void newSchedule( uint8_t priorityTracker[], int8_t prioritizedTask[][NUM_PRIORITIES])      // creating priority schedule
{
    int theTask, level;

    for(theTask = 0; theTask < taskCount; theTask ++)
    {

        for(level = 0; level < NUM_PRIORITIES; level ++)
        {
              if(tcb[theTask].currentPriority == level )
              {
                 prioritizedTask[theTask][level] = theTask;                                  // assign the task its priority level

                 priorityTracker[level]++;                                                   // know the accurate amount of tasks in each level
                 break;
               }
        }
    }
}

// DONE: Implement prioritization to NUM_PRIORITIES
uint8_t rtosScheduler(void)
{
    bool ok;
    bool renewSchedule = false;
    static bool firstTime = true;
    static int8_t prioritizedTask[MAX_TASKS][NUM_PRIORITIES] = { [0 ... MAX_TASKS-1][0 ... NUM_PRIORITIES-1] = -1 };    // initialized all to -1
    static uint8_t priorityTracker[NUM_PRIORITIES]={0};                                                                 // keep track where tasks have priorities
    static uint8_t taskOrder [NUM_PRIORITIES] = { [0 ... NUM_PRIORITIES-1] = 0xFF };                                    // initialize them all with 0xFF so we can start from zero when entering the decision loop                                                                          // keep track of the tasks execution order

    int theTask,level;

    if(priorityScheduler)
    {
            if(firstTime)                                                                               // only initialize the first time and when somebodies prority change
            {
                newSchedule(priorityTracker,prioritizedTask);                                           // create initial schedule

                    firstTime = false;
            }

            else for(theTask = 0; theTask < MAX_TASKS;theTask++)
                {
                     if(tcb[theTask].priority != tcb[theTask].currentPriority)
                     {
                         if(taskPriorityChanged[theTask] == true)
                         {
                             taskPriorityChanged[theTask] = false;                                           // we make sure not to create a schedule next time around
                             reinitializePriorites(prioritizedTask,taskCount,NUM_PRIORITIES);               // reset priorites to initial state to be ready for new schedule
                             renewSchedule = true;                                                          // renew the task list if somebody changes their priority level
                             break;
                         }
                     }
                }

            if(renewSchedule)                                                                           // anytime there is a different priority we will renew the schedule
            {
                for(level = 0; level < NUM_PRIORITIES; level ++) priorityTracker[level]=0;              // refresh priorities

                newSchedule(priorityTracker,prioritizedTask);                                           // create a new schedule

                renewSchedule = false;
            }


            for(level = 0; level < NUM_PRIORITIES; level ++)
            {
                if(priorityTracker[level])                                                        // if there is some tasks in the block
                {
                     for(theTask = 0; theTask < taskCount; theTask ++)
                     {

                           ok = false;                                                                      // we will round robin through these tasks
                           int count = 0;                                                                   // once we reach the total amount of tasks we go to the next loop
                           while (!ok)
                           {
                               count++;
                               taskOrder[level]++;                                                        // keeping track of the next order at that priority level
                               if (taskOrder[level] >= taskCount)                                         // start from position last left from
                                   taskOrder[level] = 0;
                               if(prioritizedTask[taskOrder[level]][level] == -1) break;                                // do nothing and go to the next loop if this task doesnt match prority level

                               ok = (tcb[prioritizedTask[taskOrder[level]][level]].state == STATE_READY);

                               if(ok) return prioritizedTask[taskOrder[level]][level];                                  // return task immediately if we found a match

                               if(count == taskCount) break;                                                // cycled through all possiblities, go to next level

                            }


                      }
                  }
              }
       }

    else if (!priorityScheduler)                                                                        // round robin
    {
        static uint8_t task = 0xFF;
        ok = false;
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY);
        }
        return task;
    }
}

// DONE: modify this function to start the operating system
// by calling scheduler, set srd bits, setting PSP, ASP bit, call fn with fn add in R0
// fn set TMPL bit, and PC <= fn
void startRtos(void)
{
    enableFaultExceptions();                                // enable faults to trigger
    setPSP(0x20008000);
    setASP();
    userMode();
    __asm__ (" SVC #0");                                    // supervisor call for running first task

}

// DONE:
// add task if room in task list
// store the thread name
// allocate stack space and store top of stack in sp and spInit
// set the srd bits based on the memory allocation
// initialize the created stack to make it appear the thread has run before
bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    void *myStackBaseAddress;
    uint32_t mySize;
    uint32_t *temp_variable;

    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}                                      // task available to be input

            tcb[i].pid = fn;                                             // to keep organized, the PID will be the same as base address
            pid = (uint32_t)tcb[i].pid;                                            // used solely for memory management
            myStackBaseAddress = mallocFromHeap(stackBytes);
            tcb[i].mySize = whatsMySizeFromBase(myStackBaseAddress);
            whatsMyName(tcb[i].name, name);                              // name of function
            tcb[i].state = STATE_READY;                                  // ready to be ran
            tcb[i].sp =  (uint32_t)myStackBaseAddress + tcb[i].mySize;       // point to top of stack
            tcb[i].spInit = tcb[i].sp ;                                  // point to top of stack
            tcb[i].priority = priority;
            tcb[i].currentPriority = priority;
            tcb[i].srd = createNoSramAccessMask();
            addSramAccessWindow(&tcb[i].srd,myStackBaseAddress,tcb[i].mySize);

            temp_variable = tcb[i].pid;
            __asm__ ("PROGRAM_LOCATION: MOV R12, R0\n");                // program location to put into program counter
            appearAsRanTask(tcb[i].spInit);                             // make this task appear as ran

            tcb[i].sp = getPSP();                                       // set the new psp value

            // increment task count
            taskCount++;
            ok = true;

        }
    }
    return ok;
}

// DONE: modify this function to restart a thread
void restartThread(_fn fn)
{
    __asm__ (" SVC #8");              // Restart Thread Service Call

}

// DONE: modify this function to stop a thread
// DONE: remove any pending semaphore waiting, unlock any mutexes
void stopThread(_fn fn)
{
    __asm__ (" SVC #16");              // Stop Thread Service Call

}

// DONE: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
    __asm__ (" MOV R3, R0");                        // move variable to avoid being overwritten
    __asm__ (" MOV R12, R1");                        // move variable to avoid being overwritten

    __asm__ (" SVC #19");                            // supervisor call changing thread priority


}

// DONE: modify this function to yield execution back to scheduler using pendsv
void yield(void)
{
    __asm__ (" SVC #1");                            // supervisor call for yielding to new task

}

// DONE: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
      __asm__ (" SVC #2");             // Sleep Service Call
}

// DONE: modify this function to lock a mutex using pendsv
void lock(int8_t mutex)
{
    __asm__ (" SVC #3");              // lock a mutex SVC

}

// DONE: modify this function to unlock a mutex using pendsv
void unlock(int8_t mutex)
{
    __asm__ (" SVC #4");              // unlock a mutex SVC

}

// DONE: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    __asm__ (" SVC #5");              // waiting for a semaphore SVC

}

// DONE: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    __asm__ (" SVC #6");              // waiting for a semaphore SVC

}

// DONE: modify this function to add support for the system timer
// DONE: in preemptive code, add code to request task switch
void systickIsr(void)
{   uint8_t i;

    static uint32_t taskSwitchTime;
    static uint32_t oneSecond= 0;

    for(i = 0; i < taskCount; i++)
    {
        if(tcb[i].state == STATE_DELAYED)
        {
            tcb[i].ticks --;

            if(tcb[i].ticks == 0)
            {
                tcb[i].state = STATE_READY;
            }
        }
    }

    oneSecond ++;
    if(oneSecond == 1000)           // change buffer after 1 second
    {
        if(ping)
        {
            ping = false;                             // secure data ready to be read from
            pong = true;
            for(i = 0; i < taskCount; i++)
            {
                tcb[i].timeElaspesB = 0;              // Now writing to buffer B
                                                      // Now reading from buffer A
            }
        }
        else if (pong)
        {
            ping = true;
            pong = false;                             // secure data ready to be read from
            for(i = 0; i < taskCount; i++)
            {
                tcb[i].timeElaspesA = 0;              // Now writing to buffer A
                                                      // Now reading from buffer B
            }
        }
        oneSecond = 0;                                  // wait for the next second te update buffer.
    }
    if(preemption)
    {
            NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;              // go task switch

    }
}

// DONE: in coop and preemptive, modify this function to add support for task switching
// DONE: process UNRUN and READY tasks differently
void pendSvIsr(void)
{
        // keep track of task CPU time
        WTIMER3_CTL_R &= ~TIMER_CTL_TBEN;                                   // turn-off counter
        if(ping) tcb[taskCurrent].timeElaspesA += WTIMER3_TBV_R;            // write in buffer A
        else if (pong) tcb[taskCurrent].timeElaspesB += WTIMER3_TBV_R;      // write in buffer B

        // task switch
        NVIC_INT_CTRL_R &= ~NVIC_INT_CTRL_PEND_SV;

        saveR4toR11(getPSP());                                              // save Register values
        tcb[taskCurrent].sp = getPSP();

        taskCurrent = rtosScheduler();                                      // what is my new task to be ran?
        applySramAccessMask(tcb[taskCurrent].srd);                          // enable this region of memory

        WTIMER3_TBV_R = 0;
        WTIMER3_CTL_R |= TIMER_CTL_TBEN;                                    // turn timer back on

        taskSwitch(tcb[taskCurrent].sp);

}

// DONE: modify this function to add support for the service call
// DONE: in preemptive code, add code to handle synchronization primitives
void svCallIsr(void)
{
    uint8_t svCallNumber = getSVCallNumber();
    uint8_t i, good;
    uint32_t myTicks;                              // PSP points at R0 which the Ticks Value is Saved
    uint32_t mySemaphore;                          // will be located in R0 of the PSP
    uint32_t myMUTEX;                              // will be located in R0 of the PSP
    uint32_t *sizeOfMemory;                        // will be located in R0 of the PSP
    uint32_t *myName;                              // will be located in R0 of the PSP
    uint32_t *myPID;                               // will be located in R0 of the PSP
    uint32_t  mySize;
    uint64_t temp;
    uint8_t taskNumber,j,k;
    void *myAddress;

    char pidPrint[10];
    uint8_t index;
    pidInfo *pidInfoNeeded;
    ipcsInfo *ipcsInfoNeeded;
    // Cannot declare variables inside of case statements
    switch (svCallNumber)
        {
            case START_RTOS:
                                taskCurrent = rtosScheduler();                          // what is my first task to be ran?
                                applySramAccessMask(tcb[taskCurrent].srd);              // enable this region of memory
                                runFirstTask(tcb[taskCurrent].sp);                      // go to the top of the stack where we "was running"
                                break;

            case YIELD:
                                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
                                break;

            case SLEEP:
                                myTicks = *getPSP();
                                tcb[taskCurrent].ticks = myTicks;
                                tcb[taskCurrent].state = STATE_DELAYED;
                                NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;
                                break;

            case LOCK_MUTEX:
                                myMUTEX = *getPSP();

                                if(mutexes[myMUTEX].lock == false)                                                              // mutex is available
                                {
                                    tcb[taskCurrent].mutex = myMUTEX;
                                    mutexes[myMUTEX].lock = true;
                                    mutexes[myMUTEX].lockedBy = taskCurrent;

                                }
                                else if(mutexes[myMUTEX].lock == true &&  mutexes[myMUTEX].queueSize < MAX_MUTEX_QUEUE_SIZE)    // mutex is available
                                {
                                    tcb[taskCurrent].state = STATE_BLOCKED_MUTEX;
                                    tcb[taskCurrent].mutex = myMUTEX;                                                           // which mutex am i waiting on?
                                    mutexes[myMUTEX].processQueue[mutexes[myMUTEX].queueSize] = taskCurrent;                    // now waiting in line for the next availability
                                    mutexes[myMUTEX].queueSize++;
                                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;                                                   // go task switch

                                }


                                break;

            case UNLOCK_MUTEX:
                                myMUTEX = *getPSP();

                                if(mutexes[myMUTEX].lockedBy == taskCurrent && mutexes[myMUTEX].queueSize == 0 )                // the correct ID must be trying to unlock the MUTEX
                                {
                                    mutexes[myMUTEX].lock = false;                                                              // mutex is now unlocked
                                }
                                else if(mutexes[myMUTEX].lockedBy == taskCurrent && mutexes[myMUTEX].queueSize <= MAX_MUTEX_QUEUE_SIZE)
                                {
                                    mutexes[myMUTEX].lock = true;                                                               // mutex is now unlocked
                                    mutexes[myMUTEX].lockedBy = mutexes[myMUTEX].processQueue[0];
                                    tcb[mutexes[myMUTEX].processQueue[0]].state = STATE_READY;                                  // the next task is now ready to be ran and use the resource

                                    for(i = 0 ; i < MAX_MUTEX_QUEUE_SIZE - 1 ; i++)
                                    {
                                        mutexes[myMUTEX].processQueue[i] = mutexes[myMUTEX].processQueue[i+1];
                                    }

                                    mutexes[myMUTEX].queueSize--;
                                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;                             // go task switch

                                }
                                else
                                {
                                    stopThread(tcb[taskCurrent].pid);
                                    putsUart0("You can't do that.. Your gone\n");
                                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;                             // go task switch

                                }

                                break;

            case WAIT_SEMAPHORE:
                                mySemaphore = *getPSP();

                                if(semaphores[mySemaphore].count > 0)
                                {
                                    semaphores[mySemaphore].count--;                                       // still have some semaphores to hand out
                                    return;
                                }
                                else
                                {
                                    semaphores[mySemaphore].processQueue[semaphores[mySemaphore].queueSize] = taskCurrent;     // assign a semaphore to this task
                                    semaphores[mySemaphore].queueSize++;                                                       // que size is increased cause now a task is waiting for it
                                    tcb[taskCurrent].semaphore = mySemaphore;                                                  // which semaphore am I waiting on?
                                    tcb[taskCurrent].state = STATE_BLOCKED_SEMAPHORE;                                          // cant run until a semaphore has been posted
                                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;                                                  // go task switch
                                }
                                break;

            case POST_SEMAPHORE:
                                mySemaphore = *getPSP();

                                semaphores[mySemaphore].count++;                                                                // semaphore now ready to be used

                                if(semaphores[mySemaphore].queueSize > 0 && semaphores[mySemaphore].queueSize <= MAX_SEMAPHORE_QUEUE_SIZE)                                                  // if a process is waiting for the semaphore ... Allow it to be used
                                {
                                    tcb[semaphores[mySemaphore].processQueue[0]].state = STATE_READY;                           // this task is no longer blocked by semaphore


                                    for(i = 0 ; i < MAX_SEMAPHORE_QUEUE_SIZE - 1 ; i++)                                         // move line forward
                                    {
                                        semaphores[mySemaphore].processQueue[i] = semaphores[mySemaphore].processQueue[i+1];
                                    }
                                    semaphores[mySemaphore].queueSize--;
                                    NVIC_INT_CTRL_R |= NVIC_INT_CTRL_PEND_SV;                                                   // go task switch

                                }
                                break;

            case ALLOCATE_MEMORY:
                                sizeOfMemory = getPSP();
                                pid = (uint32_t)tcb[taskCurrent].pid;                                       // make sure the memory the PID gets stored into the memory management
                                myAddress = mallocFromHeap(*sizeOfMemory);
                                mySize = whatsMySizeFromBase(myAddress);
                                addSramAccessWindow(&tcb[taskCurrent].srd,myAddress,mySize);
                                applySramAccessMask(tcb[taskCurrent].srd);                                  // enable this region of memory
                                *sizeOfMemory = myAddress;
                                gobacktoPSP();


                                break;

            case SCHEDULE:
                                    priorityScheduler = *getPSP();                                  // called from the shell and will have the value stored in R0
                                                                                                    // yield set in the shell
                                    gobacktoPSP();

                                break;

            case PREEMPTION:
                                    preemption = *getPSP();                                         // called from the shell and will have the value stored in R0

                                    gobacktoPSP();

                                break;

            case PID_FROM_NAME:
                                good = 0;
                                myName = getPSP();
                                for( i = 0; i < taskCount; i ++)                                    // cycle through to find the correct PID
                                {
                                    if(compare_2words(tcb[i].name, *myName) )                       // if we found the name return the PID
                                    {                                                               // the name is stored as a pointer in PSP's R0 right before we call the SVC ISR
                                        *myName =  tcb[i].pid;                                      // place the value of PID in R0 to be able to access value upon returning
                                        good = 1;
                                        break;                                                      // get out of the loop
                                    }

                                }
                                if(!good) *myName = myReturnPrintValue;                             // control what gets printed in the shell
                                gobacktoPSP();
                                break;

            case KILL_BY_PID:

                                good = 0;
                                myReturnPrintValue = 0;                                             // handles the case of the pid not existing at all
                                myPID = getPSP();
                                for( i = 0; i < taskCount; i ++)                                    // cycle through to find the correct PID
                                {
                                    if(tcb[i].pid == *myPID)                                        // if we found the name return the PID
                                    {                                                               // the name is stored as a pointer in PSP's R0 right before we call the SVC ISR

                                        stoppingThreadNow(tcb[i].pid);                                     // stop the thread from running
                                        good = 1;                                                   // we found a match
                                        break;                                                      // get out of the loop
                                    }
                                }
                                *myPID = myReturnPrintValue;                                        // control what gets printed in the shell
                                gobacktoPSP();

                                break;

            case KILL_BY_NAME:
                                good = 0;
                                myReturnPrintValue = 0;                                             // handles the case of the pid not existing at all
                                myName = getPSP();
                                for( i = 0; i < taskCount; i ++)                                    // cycle through to find the correct PID
                                {
                                    if(compare_2words(tcb[i].name, *myName) )                       // if we found the name return the PID
                                    {                                                               // the name is stored as a pointer in PSP's R0 right before we call the SVC ISR
                                        stoppingThreadNowByName(tcb[i].pid);                                     // stop the thread from running
                                        good = 1;                                                   // we found a match
                                        break;                                                      // get out of the loop
                                    }
                                }
                                *myName = myReturnPrintValue ;                                      // control what gets printed in the shell
                                gobacktoPSP();

                                break;

            case PROCESS_INFO:
                                pidInfoNeeded = returnR12();

                                if(!ping)                                                                     // balls not on my side ( writing information)
                                {                                                                             // therefore we are ready to read secure data
                                    totalCpuTimeA = 0;

                                    for(i = 0; i < taskCount; i++)
                                    {
                                         totalCpuTimeA += tcb[i].timeElaspesA;
                                    }
                                    for(i = 0; i < taskCount; i++)
                                    {
                                        temp = tcb[i].timeElaspesA *10000;                                   // offset the variable so no floating printing
                                        tcb[i].cpuUsage = temp / totalCpuTimeA;                              // will type the decimal point instead

                                    }


                                }
                                else if (!pong)
                                {
                                    totalCpuTimeB = 0;

                                    for(i = 0; i < taskCount; i++)
                                    {
                                         totalCpuTimeB += tcb[i].timeElaspesB;
                                    }
                                    for(i = 0; i < taskCount; i++)
                                    {
                                        temp = tcb[i].timeElaspesB *10000;                                   // offset the variable so no floating printing
                                        tcb[i].cpuUsage = temp / totalCpuTimeB;                              // will type the decimal point instead
                                    }
                                }



                                for( i = 0; i < taskCount; i ++)                                             // cycle through only the amount of PID's available
                                {
                                    whatsMyName(pidInfoNeeded[i].name, tcb[i].name);
                                    pidInfoNeeded[i].cpuUsage = tcb[i].cpuUsage;
                                    pidInfoNeeded[i].state = tcb[i].state;
                                    pidInfoNeeded[i].mutex = tcb[i].mutex;
                                    pidInfoNeeded[i].semaphore = tcb[i].semaphore;
                                    pidInfoNeeded[i].ticks = tcb[i].ticks;
                                    pidInfoNeeded[i].currentPriority = tcb[i].currentPriority;
                                    pidInfoNeeded[i].tatalTaskCount = taskCount;

                                }
                                gobacktoPSP();

                                break;

            case STOP_THREAD:

                                myPID = getPSP();
                                stoppingThreadNow(*myPID);
                                gobacktoPSP();

                                break;

            case RESTART_THREAD:
                                myPID = getPSP();
                                restartingThreadNow(*myPID);
                                gobacktoPSP();

                                break;

            case RESTART_BY_NAME:
                                myName = getPSP();
                                for( i = 0; i < taskCount; i ++)                                    // cycle through to find the correct PID
                                {
                                    if(compare_2words(tcb[i].name, *myName) )                       // if we found the name return the PID
                                    {
                                        restartingThreadNowByName(tcb[i].pid);
                                        break;                                                      // get out of the loop
                                    }
                                }
                                gobacktoPSP();

                                break;

            case MEMORY_INFO:
                                    pidInfoNeeded = returnR12();

                                    for( i = 0; i < taskCount; i ++)                                            // cycle through only the amount of PID's available
                                    {
                                        whatsMyName(pidInfoNeeded[i].name, tcb[i].name);
                                        pidInfoNeeded[i].stackBaseAdress = tcb[i].spInit -tcb[i].mySize;
                                        pidInfoNeeded[i].stackMemoryAmount = tcb[i].mySize;

                                        pidInfoNeeded[i].tatalTaskCount = taskCount;

                                        pidInfoNeeded[i].heapBaseAdress = do_i_have_extra_memory(tcb[i].pid);
                                        if(pidInfoNeeded[i].heapBaseAdress)
                                        {
                                            pidInfoNeeded[i].heapMemoryAmount =  whatsTheSizeOfThatExtraMemory(tcb[i].pid);

                                        }
                                        else pidInfoNeeded[i].heapMemoryAmount = 0;


                                    }
                                    gobacktoPSP();

                                break;


            case CHANGE_PRIORITY:
                                     myPID = returnR3();
                                     temp = returnR12();
                                     good = 0;
                                     if(temp < 0 || temp > 15)
                                     {
                                         putsUart0("Sorry We Cant Do That");
                                         gobacktoPSP();

                                     }
                                     else
                                     {
                                          for( i = 0; i < taskCount; i ++)                                    // cycle through to find the correct PID
                                          {
                                              if(tcb[i].pid == myPID)                                         // if we found the name return the PID
                                              {                                                               // the name is stored as a pointer in PSP's R0 right before we call the SVC ISR
                                                  tcb[i].currentPriority = temp;
                                                  taskPriorityChanged[i] = true;                              // lets the scheduler know that this is a new thread priority
                                                  good = 1;
                                                  break;                                                      // get out of the loop
                                              }
                                          }

                                     }
                                     if(!good)
                                     {
                                         putsUart0("Invalid PID...\n ");

                                     }
                                     if(tcb[i].currentPriority == tcb[i].priority)
                                     {
                                         putsUart0(tcb[i].name);
                                         putsUart0("'s original priority set... ");
                                         putsUart0("\n ");
                                     }
                                     else
                                     {
                                         putsUart0(tcb[i].name);
                                         putsUart0("'s priority changed to ");
                                         index = int_to_ascii(temp, pidPrint);
                                         putsUart0(&pidPrint[index]);
                                         putsUart0("\n ");
                                     }
                                     gobacktoPSP();


                                break;
            case IPCS:
                                ipcsInfoNeeded = returnR12();


                                for(i = 0; i < MAX_MUTEXES ; i ++)
                               {
                                   ipcsInfoNeeded->lock[i] = mutexes[i].lock;
                                   ipcsInfoNeeded->mutexLockedBy[i] = mutexes[i].lockedBy;
                                   ipcsInfoNeeded->mutexQueSize[i] = mutexes[i].queueSize;

                                   for(j = 0; j < MAX_MUTEX_QUEUE_SIZE ; j ++)
                                  {
                                       ipcsInfoNeeded->mutexProcessQue[i][j] =  mutexes[i].processQueue[j];


                                  }
                               }

                                for(i = 0; i < MAX_SEMAPHORES ; i ++)
                                {
                                    ipcsInfoNeeded->semaphoreCount[i] =  semaphores[i].count;
                                    ipcsInfoNeeded->semaphoreQueSize[i] =  semaphores[i].queueSize;

                                    for(j = 0; j < MAX_SEMAPHORE_QUEUE_SIZE ; j ++)
                                    {
                                        ipcsInfoNeeded->semaphoreProcessQue[i][j] =  semaphores[i].processQueue[j];

                                    }


                                }

                                gobacktoPSP();
                                break;


            case SHELL_BASE_ADDRESS:
                                sizeOfMemory = getPSP();
                                for( i = 0; i < taskCount; i ++)                                    // cycle through to find the correct PID
                                {
                                    if(compare_2words(tcb[i].name, "Shell") )                       // if we found the name return the PID
                                    {                                                               // the name is stored as a pointer in PSP's R0 right before we call the SVC ISR
                                        *sizeOfMemory = (uint32_t)tcb[i].spInit - tcb[i].mySize;
                                        break;
                                    }
                                }
                                gobacktoPSP();

                                break;

            case REBOOT:
                                NVIC_APINT_R = NVIC_APINT_SYSRESETREQ | NVIC_APINT_VECTKEY;       // key allows the reset register to be written to

                                break;


        }

}

void switchingTasks(void)
{
    NVIC_INT_CTRL_R &= NVIC_INT_CTRL_PEND_SV;

    saveR4toR11(getPSP());                                              // save Register values
    tcb[taskCurrent].sp = getPSP();

    taskCurrent = rtosScheduler();                                      // what is my new task to be ran?
    setPSP(tcb[taskCurrent].sp);
    applySramAccessMask(tcb[taskCurrent].srd);                          // enable this region of memory

    taskSwitch(tcb[taskCurrent].sp);

}

void stopCurrentTask(uint32_t address)
{

    uint8_t  good;
    uint8_t taskNumber,j,k,index;
    char pidPrint[10];

    for( taskNumber = 0; taskNumber < taskCount; taskNumber ++)                                    // cycle through to find the correct PID
    {
        if( address < (uint32_t)tcb[taskNumber].spInit  &&  address >= ((uint32_t)tcb[taskNumber].spInit -tcb[taskNumber].mySize)  )
        {
            good = true;                                              // we found a match
            break;                                                    // get out of the loop
        }
    }
       if (!good)
       {
               putsUart0("PID: \"");
               index = int_to_ascii(tcb[taskNumber].pid, pidPrint);
               putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
               putsUart0("\" unknown\n");
       }
       else if (good && (tcb[taskNumber].state == STATE_STOPPED))
       {
               index = int_to_ascii(tcb[taskNumber].pid, pidPrint);
               putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
               putsUart0(" isn't running \n");
       }
       else
       {
   ////////////////////////////////check if there are any mutexes or if waiting in line for one ////////////////////////////////////////

           if (tcb[taskNumber].state == STATE_BLOCKED_MUTEX)                                   // i was waiting on the mutex but now im getting out of line
           {
               for(j = 0 ; j < MAX_MUTEX_QUEUE_SIZE - 1 ; j++)
               {
                   if( mutexes[tcb[taskNumber].mutex].processQueue[j] == taskNumber)        // if i find my task number then we will move the line forward from there
                   {
                       mutexes[tcb[taskNumber].mutex].processQueue[j] = mutexes[tcb[taskNumber].mutex].processQueue[j+1];
                   }
               }
               mutexes[tcb[taskNumber].mutex].queueSize--;

           }
           else if(mutexes[tcb[taskNumber].mutex].lockedBy == taskNumber && mutexes[tcb[taskNumber].mutex].queueSize > 0)        // im blocking the mutex and someone else is in line waiting for it
           {
               mutexes[tcb[taskNumber].mutex].lock = true;                                                        // mutex is now unlocked
               mutexes[tcb[taskNumber].mutex].lockedBy = mutexes[tcb[taskNumber].mutex].processQueue[0];
               tcb[mutexes[tcb[taskNumber].mutex].processQueue[0]].state = STATE_READY;                           // the next task is now ready to be ran and use the resource

               for(j = 0 ; j < MAX_MUTEX_QUEUE_SIZE - 1 ; j++)
               {
                   mutexes[tcb[taskNumber].mutex].processQueue[j] = mutexes[tcb[taskNumber].mutex].processQueue[j+1];
               }

               mutexes[tcb[taskNumber].mutex].queueSize--;

           }
           else if (mutexes[tcb[taskNumber].mutex].lockedBy == taskNumber )          // im blocking the mutex so I need to let it go
           {
               mutexes[tcb[taskNumber].mutex].lock = false;                      // mutex is now unlocked
           }

   ////////////////////////////////check if there are any semaphores or if waiting in line for one ////////////////////////////////////

           if (tcb[taskNumber].state == STATE_BLOCKED_SEMAPHORE && semaphores[tcb[taskNumber].semaphore].queueSize > 0)                                                                      // i was waiting , now just remove me from the line
           {

               for(j = 0 ; j < MAX_SEMAPHORE_QUEUE_SIZE - 1 ; j++)                                                                     // move line forward
               {

                   if( semaphores[tcb[taskNumber].semaphore].processQueue[j] == taskNumber)                                            // if i find my task number then we will move the line forward from there
                   {
                       for(k = j; k < MAX_SEMAPHORE_QUEUE_SIZE - 1; k++)
                       {
                           semaphores[tcb[taskNumber].semaphore].processQueue[k] = semaphores[tcb[taskNumber].semaphore].processQueue[k+1];      // move up line from this location forward
                       }
                       break;                                                                                                                  // leave the loop
                   }
               }
               semaphores[tcb[taskNumber].semaphore].queueSize--;
           }

   ////////////////////////////////////free up the memory/////////////////////////////////////////////////////////

           tcb[taskNumber].state = STATE_STOPPED;                   // set state to stopped
           pid = (uint32_t)tcb[taskNumber].pid;                               // used to check if this is the correct pid to free the memory
           freeToHeap(tcb[taskNumber].spInit - tcb[taskNumber].mySize);      // remove memory based on the base address
           tcb[taskNumber].sp = 0;
           tcb[taskNumber].spInit = 0;
           tcb[taskNumber].srd = createNoSramAccessMask();          // has no access anywhere in memory

           putsUart0("PID ");
           num_to_HEX (pid, pidPrint);

           putsUart0(pidPrint);                             // start at this index to remove leading zeroes
           putsUart0(" killed\n");
       }

}

void stoppingThreadNow(uint32_t myPID)
{
    uint8_t  good;
    uint8_t taskNumber,j,k,index;
    char pidPrint[10];

    for( taskNumber = 0; taskNumber < taskCount; taskNumber ++)                                    // cycle through to find the correct PID
    {
        if(tcb[taskNumber].pid == myPID)
        {
            good = true;                                              // we found a match
            break;                                                    // get out of the loop
        }
    }
    if (!good)
    {
            putsUart0("PID: \"");
            index = int_to_ascii(myPID, pidPrint);
            putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
            putsUart0("\" unknown\n");
    }
    else if (good && (tcb[taskNumber].state == STATE_STOPPED))
    {
            index = int_to_ascii(myPID, pidPrint);
            putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
            putsUart0(" isn't running \n");
    }
    else
    {
////////////////////////////////check if there are any mutexes or if waiting in line for one ////////////////////////////////////////

        if (tcb[taskNumber].state == STATE_BLOCKED_MUTEX)                                   // i was waiting on the mutex but now im getting out of line
        {
            for(j = 0 ; j < MAX_MUTEX_QUEUE_SIZE - 1 ; j++)
            {
                if( mutexes[tcb[taskNumber].mutex].processQueue[j] == taskNumber)        // if i find my task number then we will move the line forward from there
                {
                    mutexes[tcb[taskNumber].mutex].processQueue[j] = mutexes[tcb[taskNumber].mutex].processQueue[j+1];
                }
            }
            mutexes[tcb[taskNumber].mutex].queueSize--;

        }
        else if(mutexes[tcb[taskNumber].mutex].lockedBy == taskNumber && mutexes[tcb[taskNumber].mutex].queueSize > 0)        // im blocking the mutex and someone else is in line waiting for it
        {
            mutexes[tcb[taskNumber].mutex].lock = true;                                                        // mutex is now unlocked
            mutexes[tcb[taskNumber].mutex].lockedBy = mutexes[tcb[taskNumber].mutex].processQueue[0];
            tcb[mutexes[tcb[taskNumber].mutex].processQueue[0]].state = STATE_READY;                           // the next task is now ready to be ran and use the resource

            for(j = 0 ; j < MAX_MUTEX_QUEUE_SIZE - 1 ; j++)
            {
                mutexes[tcb[taskNumber].mutex].processQueue[j] = mutexes[tcb[taskNumber].mutex].processQueue[j+1];
            }

            mutexes[tcb[taskNumber].mutex].queueSize--;

        }
        else if (mutexes[tcb[taskNumber].mutex].lockedBy == taskNumber )          // im blocking the mutex so I need to let it go
        {
            mutexes[tcb[taskNumber].mutex].lock = false;                      // mutex is now unlocked
        }

////////////////////////////////check if there are any semaphores or if waiting in line for one ////////////////////////////////////

        if (tcb[taskNumber].state == STATE_BLOCKED_SEMAPHORE && semaphores[tcb[taskNumber].semaphore].queueSize > 0)                                                                      // i was waiting , now just remove me from the line
        {

            for(j = 0 ; j < MAX_SEMAPHORE_QUEUE_SIZE - 1 ; j++)                                                                     // move line forward
            {

                if( semaphores[tcb[taskNumber].semaphore].processQueue[j] == taskNumber)                                            // if i find my task number then we will move the line forward from there
                {
                    for(k = j; k < MAX_SEMAPHORE_QUEUE_SIZE - 1; k++)
                    {
                        semaphores[tcb[taskNumber].semaphore].processQueue[k] = semaphores[tcb[taskNumber].semaphore].processQueue[k+1];      // move up line from this location forward
                    }
                    break;                                                                                                                  // leave the loop
                }
            }
            semaphores[tcb[taskNumber].semaphore].queueSize--;
        }

////////////////////////////////////free up the memory/////////////////////////////////////////////////////////

        tcb[taskNumber].state = STATE_STOPPED;                   // set state to stopped
        pid = (uint32_t)tcb[taskNumber].pid;                               // used to check if this is the correct pid to free the memory
        freeToHeap(tcb[taskNumber].spInit - tcb[taskNumber].mySize);      // remove memory based on the base address
        tcb[taskNumber].sp = 0;
        tcb[taskNumber].spInit = 0;
        tcb[taskNumber].srd = createNoSramAccessMask();          // has no access anywhere in memory


        index = int_to_ascii(myPID, pidPrint);
        putsUart0(&pidPrint[index]);                    	     // start at this index to remove leading zeroes
        putsUart0(" killed\n");
    }
}

void stoppingThreadNowByName(uint32_t myPID)
{
    uint8_t  good;
    uint8_t taskNumber,j,k;
    char pidPrint[10];

    for( taskNumber = 0; taskNumber < taskCount; taskNumber ++)                                    // cycle through to find the correct PID
    {
        if(tcb[taskNumber].pid == myPID)
        {
            good = true;                                              // we found a match
            break;                                                    // get out of the loop
        }
    }
    if (!good)
    {
            putsUart0("Task \"");
            putsUart0(tcb[taskNumber].name);
            putsUart0("\" is unknown\n");
    }
    else if (good && (tcb[taskNumber].state == STATE_STOPPED))
    {
            putsUart0(tcb[taskNumber].name);                  // start at this index to remove leading zeroes
            putsUart0(" isn't running \n");
    }
    else
    {
////////////////////////////////check if there are any mutexes or if waiting in line for one ////////////////////////////////////////

        if (tcb[taskNumber].state == STATE_BLOCKED_MUTEX)                                   // i was waiting on the mutex but now im getting out of line
        {
            for(j = 0 ; j < MAX_MUTEX_QUEUE_SIZE - 1 ; j++)
            {
                if( mutexes[tcb[taskNumber].mutex].processQueue[j] == taskNumber)        // if i find my task number then we will move the line forward from there
                {
                    mutexes[tcb[taskNumber].mutex].processQueue[j] = mutexes[tcb[taskNumber].mutex].processQueue[j+1];
                }
            }
            mutexes[tcb[taskNumber].mutex].queueSize--;

        }
        else if(mutexes[tcb[taskNumber].mutex].lockedBy == taskNumber && mutexes[tcb[taskNumber].mutex].queueSize > 0)        // im blocking the mutex and someone else is in line waiting for it
        {
            mutexes[tcb[taskNumber].mutex].lock = true;                                                        // mutex is now unlocked
            mutexes[tcb[taskNumber].mutex].lockedBy = mutexes[tcb[taskNumber].mutex].processQueue[0];
            tcb[mutexes[tcb[taskNumber].mutex].processQueue[0]].state = STATE_READY;                           // the next task is now ready to be ran and use the resource

            for(j = 0 ; j < MAX_MUTEX_QUEUE_SIZE - 1 ; j++)
            {
                mutexes[tcb[taskNumber].mutex].processQueue[j] = mutexes[tcb[taskNumber].mutex].processQueue[j+1];
            }

            mutexes[tcb[taskNumber].mutex].queueSize--;

        }
        else if (mutexes[tcb[taskNumber].mutex].lockedBy == taskNumber )          // im blocking the mutex so I need to let it go
        {
            mutexes[tcb[taskNumber].mutex].lock = false;                      // mutex is now unlocked
        }

////////////////////////////////check if there are any semaphores or if waiting in line for one ////////////////////////////////////

        if (tcb[taskNumber].state == STATE_BLOCKED_SEMAPHORE && semaphores[tcb[taskNumber].semaphore].queueSize > 0)                                                                      // i was waiting , now just remove me from the line
        {

            for(j = 0 ; j < MAX_SEMAPHORE_QUEUE_SIZE - 1 ; j++)                                                                     // move line forward
            {

                if( semaphores[tcb[taskNumber].semaphore].processQueue[j] == taskNumber)                                            // if i find my task number then we will move the line forward from there
                {
                    for(k = j; k < MAX_SEMAPHORE_QUEUE_SIZE - 1; k++)
                    {
                        semaphores[tcb[taskNumber].semaphore].processQueue[k] = semaphores[tcb[taskNumber].semaphore].processQueue[k+1];      // move up line from this location forward
                    }
                    break;                                                                                                                  // leave the loop
                }
            }
            semaphores[tcb[taskNumber].semaphore].queueSize--;
        }

////////////////////////////////////free up the memory/////////////////////////////////////////////////////////

        tcb[taskNumber].state = STATE_STOPPED;                   // set state to stopped
        pid = (uint32_t)tcb[taskNumber].pid;                               // used to check if this is the correct pid to free the memory
        freeToHeap(tcb[taskNumber].spInit - tcb[taskNumber].mySize);      // remove memory based on the base address
        tcb[taskNumber].sp = 0;
        tcb[taskNumber].spInit = 0;
        tcb[taskNumber].srd = createNoSramAccessMask();          // has no access anywhere in memory


        putsUart0(tcb[taskNumber].name);
        putsUart0(" killed\n");
    }



}

void restartingThreadNow(uint32_t myPID)
{
    uint8_t taskNumber,j,index,temp;
    void *myAddress;
    char pidPrint[10];
    bool good = false;

    for( taskNumber = 0; taskNumber < taskCount; taskNumber ++)                                    // cycle through to find the correct PID
    {
        if(tcb[taskNumber].pid == myPID)
        {
            good = true;                                              // we found a match
            break;                                                    // get out of the loop
        }
    }
    if (!good)
    {
            putsUart0("PID: \"");
            index = int_to_ascii(myPID, pidPrint);
            putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
            putsUart0("\" unknown\n");
    }
    else if(good && (tcb[taskNumber].state == STATE_STOPPED))
    {



        pid = (uint32_t)tcb[taskNumber].pid;                                                       // used for updating the data base for memory allocation and freeing
        myAddress = mallocFromHeap(tcb[taskNumber].mySize);

        if(myAddress == NULL)
        {
            putsUart0("Wait until some more memory is freed up");
            return;
        }

        tcb[taskNumber].spInit =  (uint32_t)myAddress + tcb[taskNumber].mySize;
        addSramAccessWindow(&tcb[taskNumber].srd,myAddress,tcb[taskNumber].mySize);

        tcb[taskNumber].state = STATE_READY;

        temp = (uint32_t)tcb[taskNumber].pid;
        __asm__ (" MOV R3, R0\n");                                              // will replace the PC register

        tcb[taskNumber].sp = appearAsRanTaskAgain(tcb[taskNumber].spInit);       // make this task appear as ran

        putsUart0("PID: ");
        index = int_to_ascii(myPID, pidPrint);
        putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
        putsUart0("restarted...\n");
    }
    else
    {
//        putsUart0("PID: \"");
//        index = int_to_ascii(myPID, pidPrint);
//        putsUart0(&pidPrint[index]);                    // start at this index to remove leading zeroes
//        putsUart0("\" already running!\n");
    }

}

void restartingThreadNowByName(uint32_t myPID)
{
    uint8_t taskNumber,j,index;
    uint32_t temp;
    void *myAddress;
    bool good = false;

    for( taskNumber = 0; taskNumber < taskCount; taskNumber ++)                                    // cycle through to find the correct PID
    {
        if(tcb[taskNumber].pid == myPID)
        {
            good = true;                                              // we found a match
            break;                                                    // get out of the loop
        }
    }
    if (!good)
    {
            putsUart0("Task \"");
            putsUart0(tcb[taskNumber].name);
            putsUart0("\" is unknown\n");
    }
    else if(good && (tcb[taskNumber].state == STATE_STOPPED))
    {



        pid = (uint32_t)tcb[taskNumber].pid;                                                       // used for updating the data base for memory allocation and freeing
        myAddress = mallocFromHeap(tcb[taskNumber].mySize);
        if(myAddress == NULL)
        {
            putsUart0("Wait until some more memory is freed up");
            return;
        }

        tcb[taskNumber].spInit =  (uint32_t)myAddress + tcb[taskNumber].mySize;
        addSramAccessWindow(&tcb[taskNumber].srd,myAddress,tcb[taskNumber].mySize);

        tcb[taskNumber].state = STATE_READY;

        temp = (uint32_t)tcb[taskNumber].pid;
        __asm__ (" MOV R3, R0\n");                                              // will replace the PC register

        tcb[taskNumber].sp = appearAsRanTaskAgain(tcb[taskNumber].spInit);       // make this task appear as ran

        putsUart0(tcb[taskNumber].name);
        putsUart0(" restarted...\n");
    }
    else
    {
        putsUart0(tcb[taskNumber].name);
        putsUart0(" is already running!\n");
    }

}
