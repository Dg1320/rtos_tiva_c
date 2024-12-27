// Kernel functions
// DeZean Gardner

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef KERNEL_H_
#define KERNEL_H_

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// pid information
typedef struct pidInfo_
{
    char name[16];                 // task name
    uint32_t cpuUsage;                // % of CPU time task using
    uint32_t stackMemoryAmount;         // size of Memory Allocated  *use whatsMySize
    uint32_t heapMemoryAmount;         // size of Memory Allocated  *use whatsMySize
    uint32_t stackBaseAdress;
    uint32_t heapBaseAdress;
    uint8_t state;                 // state of the task at the moment
    uint8_t mutex;                 // print only if blocked by mutex
    uint8_t semaphore;             // print only if blocked by semaphore
    uint32_t ticks;                // print only if in sleep mode*
    uint8_t currentPriority;       // print priority where 0=Hightest 15=Lowest
    uint8_t tatalTaskCount;        // just a value to be used for looping through amount of tasks
}pidInfo;


// pid information


// mutex
#define MAX_MUTEXES 1
#define MAX_MUTEX_QUEUE_SIZE 2
#define resource 0

// semaphore
#define MAX_SEMAPHORES 3
#define MAX_SEMAPHORE_QUEUE_SIZE 2
#define keyPressed 0
#define keyReleased 1
#define flashReq 2

// tasks
#define MAX_TASKS 12

// supervisor calls
#define START_RTOS 0
#define YIELD 1
#define SLEEP 2
#define LOCK_MUTEX 3
#define UNLOCK_MUTEX 4
#define WAIT_SEMAPHORE 5
#define POST_SEMAPHORE 6
#define ALLOCATE_MEMORY 7
#define RESTART_THREAD 8
#define SCHEDULE 9
#define PREEMPTION 10
#define PID_FROM_NAME 11
#define KILL_BY_PID 12
#define KILL_BY_NAME 13
#define PROCESS_INFO 14
#define REBOOT 15
#define STOP_THREAD  16
#define RESTART_BY_NAME  17
#define MEMORY_INFO  18
#define CHANGE_PRIORITY  19
#define IPCS  20
#define SHELL_BASE_ADDRESS  21
#define RESTART_BY_NAME  22

#define TASK_SWITCH 1
#define SYS_TICK 2
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

bool initMutex(uint8_t mutex);
bool initSemaphore(uint8_t semaphore, uint8_t count);

void initRtos(void);
void startRtos(void);

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes);
void restartThread(_fn fn);
void stopThread(_fn fn);
void setThreadPriority(_fn fn, uint8_t priority);

void yield(void);
void sleep(uint32_t tick);
void lock(int8_t mutex);
void unlock(int8_t mutex);
void wait(int8_t semaphore);
void post(int8_t semaphore);

void systickIsr(void);
void pendSvIsr(void);
void svCallIsr(void);

void switchingTasks(void);
uint8_t getSVCallNumberV2(void);
void newSchedule( uint8_t priorityTracker[], int8_t prioritizedTask[][]);
void reinitializePriorites(int8_t myArray[][],int rows, int cols);

void stoppingThreadNow(uint32_t myPID);
void stoppingThreadNowByName(uint32_t myPID);

void restartingThreadNow(uint32_t myPID);
void restartingThreadNowByName(uint32_t myPID);

void stopCurrentTask(uint32_t address);

typedef struct ipcsInfo_
{
    bool lock[MAX_MUTEXES];
    uint8_t mutexQueSize[MAX_MUTEXES];
    uint8_t mutexProcessQue[MAX_MUTEXES][MAX_MUTEX_QUEUE_SIZE];
    uint8_t mutexLockedBy[MAX_MUTEXES];
    uint8_t semaphoreCount[MAX_SEMAPHORES];
    uint8_t semaphoreQueSize[MAX_SEMAPHORES];
    uint8_t semaphoreProcessQue[MAX_SEMAPHORES][MAX_SEMAPHORE_QUEUE_SIZE];

}ipcsInfo;
#endif
