// Shell functions
// DeZean Gardner

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef SHELL_H_
#define SHELL_H_


//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void shell(void);                           // command line interface
void reboot(void);                          // reboots the microcontroller

void ps(void);                              // displays the process (thread) status
void ipcs(void);                            // display inter-process (thread) communication status
void pidof(const char name[]);              // display PID of process (thread)

void kill(uint32_t pid);                    // kills process (thread) with matching PID
void pkill(const char name[]);                           // kills the thread based on process name

void pi(bool on);                           // turns priority inheritance on/off
void preempt(bool on);                      // turns preemption on/off
void sched(bool prio_on);                   // selects priority or round-robin Scheduling

void printMyStatus(uint8_t status,uint8_t mutex, uint8_t semaphore, uint8_t ticks);
void print_CPU_usage(uint8_t printIndex, const char myPercentage[] );

void restartTaskByPID(uint32_t pid);
void restartTaskByName(const char name[]);

void meminfo(void);
void changePriority(uint32_t pid, uint8_t priority);



#endif
