// Memory manager functions
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
#include "uart0.h"
#include "mpu_control.h"
#include "kernel.h"

extern uint8_t taskCurrent ;
#define EXTRA_MEMORY_ALLOCATED 7

uint8_t region_available[5] ;                        // these values will be inc or dec depending on the
                                                     // memory being allocated or deallocated
mem_addr Region [5][8];
uint64_t subregionsV2= 0x000000FFFFFFFFFF;     // only 40 spots available to be chacked out

#define MAX_NAME_LENGTH 16
extern uint32_t pid;
extern struct _tcb tcb;
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------
uint32_t whatsMySizeFromStack(uint32_t *topOfStack)
{
        uint8_t i,region,offset;

        if      (((uint32_t)topOfStack > REGION0_BASE) && ((uint32_t)topOfStack <= REGION1_BASE)) region = 0;
        else if (((uint32_t)topOfStack > REGION1_BASE) && ((uint32_t)topOfStack <= REGION2_BASE)) region = 1;
        else if (((uint32_t)topOfStack > REGION2_BASE) && ((uint32_t)topOfStack <= REGION3_BASE)) region = 2;
        else if (((uint32_t)topOfStack > REGION3_BASE) && ((uint32_t)topOfStack <= REGION4_BASE)) region = 3;
        else if (((uint32_t)topOfStack > REGION4_BASE) && ((uint32_t)topOfStack <= REGION_MAX))   region = 4;



        switch (region)                     // make sure the loop check those specific regions
          {
            case 0:
                        offset = 0;
                        break;
            case 1:
                        offset = 8;
                        break;
            case 2:
                        offset = 16;
                        break;
            case 3:
                        offset = 24;
                        break;
            case 4:
                        offset = 32;
                        break;
          }

        for(i = 0; i < 8; i ++ )                                    // find the matching base address
        {
            if (Region[region][i].topOfStack == topOfStack)  return Region[region][i].size * Region[region][i].type;
        }

        return 0;

}
uint32_t whatsMySizeFromBase(uint32_t *base_Adress)
{
        uint8_t i,region,offset;


        if      (((uint32_t)base_Adress >= REGION0_BASE) && ((uint32_t)base_Adress < REGION1_BASE)) region = 0;
        else if (((uint32_t)base_Adress >= REGION1_BASE) && ((uint32_t)base_Adress < REGION2_BASE)) region = 1;
        else if (((uint32_t)base_Adress >= REGION2_BASE) && ((uint32_t)base_Adress < REGION3_BASE)) region = 2;
        else if (((uint32_t)base_Adress >= REGION3_BASE) && ((uint32_t)base_Adress < REGION4_BASE)) region = 3;
        else if (((uint32_t)base_Adress >= REGION4_BASE) && ((uint32_t)base_Adress < REGION_MAX)) region = 4;

        switch (region)                     // make sure the loop check those specific regions
          {
            case 0:
                        offset = 0;
                        break;
            case 1:
                        offset = 8;
                        break;
            case 2:
                        offset = 16;
                        break;
            case 3:
                        offset = 24;
                        break;
            case 4:
                        offset = 32;
                        break;
          }

        for(i = 0; i < 8; i ++ )                                    // find the matching base address
        {
            if (Region[region][i].heap_address == base_Adress)  return Region[region][i].size * Region[region][i].type  ;
        }

}

void whatsMyName(char *oldName, const char *newName)
{
    int Limit = 0;
    while (*newName != '\0' && Limit != MAX_NAME_LENGTH)
    {
        *oldName = *newName;    // change old name
        oldName++;              // go to next character
        newName++;              // go to next character
        Limit++;                // if we reach limit we are finished copying
    }

    *oldName = '\0';            // end on string
}

// DONE: add your malloc code here and update the SRD bits for the current thread
void *mallocFromHeap(uint32_t size_in_bytes)
{
    uint8_t memory_fit;
    void * address_given;

    if(size_in_bytes <= 512)
    {
        if(region_available[ZERO] < MAX_FIT)
        {
            memory_fit = find_open_memory(ZERO);
            address_given = (void*)(REGION0_BASE +  BASIC_MEMORY * memory_fit);
            assignMemory(Region,ZERO,memory_fit,pid,ONE,BASIC,address_given);

            return address_given;                                                    // memory assigned and can check in the struct


        }
        else if(region_available[TWO] < MAX_FIT)
        {
            memory_fit = find_open_memory(TWO);
            address_given = (void*)(REGION1_BASE +  BASIC_MEMORY * memory_fit);
            assignMemory(Region,TWO,memory_fit,pid,ONE,BASIC,address_given);

            return address_given;                                                    // memory assigned and can check in the struct

        }
        else if(region_available[THREE] < MAX_FIT)
        {
            memory_fit = find_open_memory(THREE);
            address_given = (void*)(REGION3_BASE +  BASIC_MEMORY * memory_fit);
            assignMemory(Region,THREE,memory_fit,pid,ONE,BASIC,address_given);

            return address_given;                                                    // memory assigned and can check in the struct

        }
        else
        {
            if(region_available[ONE] < MAX_FIT)
            {
                memory_fit = find_open_memory(ONE);                // if value comes back from 0to7 place it there or else value wont fit
                if(memory_fit < MAX_FIT)
                {
                    address_given = (void*)(REGION1_BASE +  FORCED_CHUNCK * memory_fit);
                    assignMemory(Region,ONE,memory_fit,pid,ONE,FORCED_CHUNCK,address_given);

                    return address_given;                                                    // memory assigned and can check in the struct

                }
            }

            else if(region_available[FOUR] < MAX_FIT)
            {
                memory_fit = find_open_memory(FOUR);                // if value comes back from 0to7 place it there or else value wont fit
                if(memory_fit < MAX_FIT)
                {
                    address_given = (void*)(REGION4_BASE +  FORCED_CHUNCK * memory_fit);
                    assignMemory(Region,FOUR,memory_fit,pid,ONE,FORCED_CHUNCK,address_given);

                    return address_given;                                                    // memory assigned and can check in the struct

                }
            }

            else
            {
                putsUart0("Sorry...No more memory available  \n");
                // no memory for 512B available
            }
        }

    }

    else if ( ( size_in_bytes > BASIC_MEMORY ) && ( size_in_bytes <= CHUNK_MEMORY ) )
    {
        if(region_available[ONE] < MAX_FIT)
        {
            memory_fit = find_open_memory(ONE);                // if value comes back from 0to7 place it there or else value wont fit
            if(memory_fit < MAX_FIT)
            {
                address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);
                assignMemory(Region,ONE,memory_fit,pid,ONE,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
        }

        else if(region_available[FOUR] < MAX_FIT)
        {
            memory_fit = find_open_memory(FOUR);                // if value comes back from 0to7 place it there or else value wont fit
            if(memory_fit < MAX_FIT)
            {
                address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);
                assignMemory(Region,FOUR,memory_fit,pid,ONE,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
        }
        else if(region_available[ZERO] < SEVEN)                                     // need at least two spots available
        {
            memory_fit = check_contiguous_memory512(ZERO,TWO);                // if value comes back from 0to16 place it there or else value wont fit

            if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION0_BASE + CHUNK_MEMORY * memory_fit);

            else return NULL;

            assignMemory(Region,ZERO,memory_fit,pid,TWO,BASIC,address_given);

            return address_given;                                                    // memory assigned and can check in the struct
        }
        else if(region_available[TWO] < MAX_FIT)                                 // just make sure we have enough to roll over to the next region
        {
            memory_fit = check_contiguous_memory512(TWO,TWO);                // if value comes back from 0to16 place it there or else value wont fit

            if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

            else return NULL;

            assignMemory(Region,TWO,memory_fit,pid,TWO,BASIC,address_given);

            return address_given;                                                    // memory assigned and can check in the struct
        }
        else if(region_available[THREE] < SEVEN)
        {
            memory_fit = check_contiguous_memory512(THREE,TWO);                // if value comes back from 0to16 place it there or else value wont fit

            if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION3_BASE + CHUNK_MEMORY * memory_fit);

            else return NULL;

            assignMemory(Region,THREE,memory_fit,pid,TWO,BASIC,address_given);

            return address_given;                                                    // memory assigned and can check in the struct
        }
        else  putsUart0("Not Enough Memory Available... Sorry\n");
    }

    else
    {
        if ( (size_in_bytes > CHUNK_MEMORY ) && (size_in_bytes <= CHUNK_MEMORY * TWO) )
        {
            if(region_available[ONE] < SEVEN)
            {
                memory_fit = check_contiguous_memory(ONE,TWO);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ONE,memory_fit,pid,TWO,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct


            }
            else if(region_available[FOUR] < SEVEN)
            {
                memory_fit = check_contiguous_memory(FOUR,TWO);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,FOUR,memory_fit,pid,TWO,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[ZERO] < FIVE)
            {
                memory_fit = check_contiguous_memory512(ZERO,FOUR);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION0_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ZERO,memory_fit,pid,FOUR,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else if(region_available[TWO] < MAX_FIT)                                 // just make sure we have enough to roll over to the next region
            {
                memory_fit = check_contiguous_memory512(TWO,FOUR);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,TWO,memory_fit,pid,FOUR,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else if(region_available[THREE] < FIVE)
            {
                memory_fit = check_contiguous_memory512(THREE,FOUR);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION3_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,THREE,memory_fit,pid,FOUR,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }

            else  putsUart0("Not Enough Memory Available... Sorry\n");

        }

        else if ( (size_in_bytes > CHUNK_MEMORY * TWO ) && (size_in_bytes <= CHUNK_MEMORY * THREE) )
        {
            if(region_available[ONE] < SIX)
            {
                memory_fit = check_contiguous_memory(ONE,THREE);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ONE,memory_fit,pid,THREE,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[FOUR] < SIX)
            {
                memory_fit = check_contiguous_memory(FOUR,THREE);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,FOUR,memory_fit,pid,THREE,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[ZERO] < THREE)
            {
                memory_fit = check_contiguous_memory512(ZERO,SIX);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION0_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ZERO,memory_fit,pid,SIX,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else if(region_available[TWO] < MAX_FIT)                                 // just make sure we have enough to roll over to the next region
            {
                memory_fit = check_contiguous_memory512(TWO,SIX);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,TWO,memory_fit,pid,SIX,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else if(region_available[THREE] < THREE)
            {
                memory_fit = check_contiguous_memory512(THREE,SIX);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION3_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,THREE,memory_fit,pid,SIX,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else  putsUart0("Not Enough Memory Available... Sorry\n");


        }

        else if ( (size_in_bytes > CHUNK_MEMORY * THREE ) && (size_in_bytes <= CHUNK_MEMORY * FOUR) )
        {
            if(region_available[ONE] < FIVE)
            {
                memory_fit = check_contiguous_memory(ONE,FOUR);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ONE,memory_fit,pid,FOUR,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[FOUR] < FIVE)
            {
                memory_fit = check_contiguous_memory(FOUR,FOUR);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,FOUR,memory_fit,pid,FOUR,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[ZERO] == ZERO)                                 // we need that entire region
            {
                memory_fit = check_contiguous_memory512(ZERO,EIGHT);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION0_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ZERO,memory_fit,pid,EIGHT,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else if(region_available[TWO] < MAX_FIT)                                 // just make sure we have enough to roll over to the next region
            {
                memory_fit = check_contiguous_memory512(TWO,EIGHT);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,TWO,memory_fit,pid,EIGHT,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else if(region_available[THREE] == ZERO)                                 // we need that entire region
            {
                memory_fit = check_contiguous_memory512(THREE,EIGHT);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION3_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,THREE,memory_fit,pid,EIGHT,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else  putsUart0("Not Enough Memory Available... Sorry\n");


        }

        else if ( (size_in_bytes > CHUNK_MEMORY * FOUR ) && (size_in_bytes <= CHUNK_MEMORY * FIVE) )
        {
            if(region_available[ONE] < FOUR)
            {
                memory_fit = check_contiguous_memory(ONE,FIVE);                // if value comes back from 0to7 place it there or else value wont fit
                if((memory_fit >=0) && (memory_fit < 8) )      address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ONE,memory_fit,pid,FIVE,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[FOUR] < FOUR)
            {
                memory_fit = check_contiguous_memory(FOUR,FIVE);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,FOUR,memory_fit,pid,FIVE,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[TWO] < SIX)                                    // need to have 2 availabe spaces to fit the entire 5k rolled over into next region
            {
                memory_fit = check_contiguous_memory512(TWO,TEN);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,TWO,memory_fit,pid,TEN,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else  putsUart0("Not Enough Memory Available... Sorry\n");


        }

        else if ( (size_in_bytes > CHUNK_MEMORY * FIVE ) && (size_in_bytes <= CHUNK_MEMORY * SIX) )
        {
            if(region_available[ONE] < THREE)
            {
                memory_fit = check_contiguous_memory(TWO,SIX);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ONE,memory_fit,pid,SIX,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[FOUR] < THREE)
            {
                memory_fit = check_contiguous_memory(FOUR,SIX);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,FOUR,memory_fit,pid,SIX,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[TWO] < FOUR)                                    // need to have 4 availabe spaces to fit the entire 6k rolled over into next region
            {
                memory_fit = check_contiguous_memory512(TWO,TWELVE);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,TWO,memory_fit,pid,TWELVE,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else  putsUart0("Not Enough Memory Available... Sorry\n");

        }

        else if ( (size_in_bytes > CHUNK_MEMORY * SIX ) && (size_in_bytes <= CHUNK_MEMORY * SEVEN) )
        {
            if(region_available[ONE] < TWO)
            {
                memory_fit = check_contiguous_memory(ONE,SEVEN);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ONE,memory_fit,pid,SEVEN,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[FOUR] < TWO)
            {
                memory_fit = check_contiguous_memory(FOUR,SEVEN);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,FOUR,memory_fit,pid,SEVEN,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[TWO] < TWO)                                    // need to have 6 available spaces to fit the entire 7k rolled over into next region
            {
                memory_fit = check_contiguous_memory512(TWO,FOURTEEN);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,TWO,memory_fit,pid,FOURTEEN,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else  putsUart0("Not Enough Memory Available... Sorry\n");

        }

        else if ( (size_in_bytes > CHUNK_MEMORY * SEVEN ) && (size_in_bytes <= CHUNK_MEMORY * EIGHT) )
        {
            if(region_available[ONE] < ONE)
            {
                memory_fit = check_contiguous_memory(TWO,EIGHT);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION1_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,ONE,memory_fit,pid,EIGHT,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[4] < ONE)
            {
                memory_fit = check_contiguous_memory(FOUR,EIGHT);                // if value comes back from 0to7 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 8))   address_given = (void*)(REGION4_BASE +  CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,FOUR,memory_fit,pid,EIGHT,CHUNK,address_given);

                return address_given;                                                    // memory assigned and can check in the struct

            }
            else if(region_available[TWO] == ZERO)                                    // need all the memory plus all the memory of the next region
            {
                memory_fit = check_contiguous_memory512(TWO,SIXTEEN);                // if value comes back from 0to16 place it there or else value wont fit

                if((memory_fit >=0) && (memory_fit < 14))   address_given = (void*)(REGION2_BASE + CHUNK_MEMORY * memory_fit);

                else return NULL;

                assignMemory(Region,TWO,memory_fit,pid,SIXTEEN,BASIC,address_given);

                return address_given;                                                    // memory assigned and can check in the struct
            }
            else  putsUart0("Not Enough Memory Available... Sorry\n");

        }

        else  putsUart0("Sorry...Thats waaay too much memory for one task\n");

    }

    return address_given;
}

void *iNeedSomeMemory(uint32_t size_in_bytes)
{
    __asm__ (" SVC #7");              // allocate out some memory

}

void *takeAwayMemory(uint32_t size_in_bytes)
{
    __asm__ (" SVC #8");              // remove memory access

}

// DONE: add your free code here and update the SRD bits for the current thread
void freeToHeap(void *pMemory)

{
    uint8_t i,j,region,offset;
    uint8_t k,l,m;
    bool offsetUpdated = false;
    bool extraMemoryFreed = false;
        char hex_display [9];



        if      (((uint32_t)pMemory >= REGION0_BASE) && ((uint32_t)pMemory < REGION1_BASE)) region = 0;
        else if (((uint32_t)pMemory >= REGION1_BASE) && ((uint32_t)pMemory < REGION2_BASE)) region = 1;
        else if (((uint32_t)pMemory >= REGION2_BASE) && ((uint32_t)pMemory < REGION3_BASE)) region = 2;
        else if (((uint32_t)pMemory >= REGION3_BASE) && ((uint32_t)pMemory < REGION4_BASE)) region = 3;
        else if (((uint32_t)pMemory >= REGION4_BASE) && ((uint32_t)pMemory < REGION_MAX)) region = 4;

        switch (region)                     // make sure the loop check those specific regions
          {
            case 0:
                        offset = 0;
                        break;
            case 1:
                        offset = 8;
                        break;
            case 2:
                        offset = 16;
                        break;
            case 3:
                        offset = 24;
                        break;
            case 4:
                        offset = 32;
                        break;
          }

        for(i = 0; i < 8; i ++ )                                    // find the matching base address
        {
            if (Region[region][i].heap_address == pMemory)
            {

                if(Region[region][i].PID != pid) putsUart0("You're trying to release memory that isn't yours :(\n");

                else
                {

                    for(j = i; j < (Region[region][i].size + i ); j ++ )      // release those adresses and update variables
                    {
                        region_available[region]--;                     // more regions available

                        subregionsV2 |= (1ULL << (j+offset)) ;                         // these subregions are now available

                    }
                    putsUart0("Stack Memory @ ");
                    num_to_HEX((uint32_t)pMemory,hex_display);
                    putsUart0(hex_display);
                    putsUart0(" size of ");
                    num_to_HEX(Region[region][i].size *  Region[region][i].type , hex_display);
                    putsUart0(hex_display);
                    putsUart0(" is now free...\n");

                    assignMemory(Region,region,i,ZERO,ZERO,ZERO,NA);
                                                                                    // find any other memory at that Pid and release it

                    for(k = 0; k < 5 ; k++)
                    {
                        offset = k*8;                                           // make sure to update the offset

                        for(l = 0; l < 8 ; l++)
                        {
                            if(Region[k][l].PID == pid)
                            {

                                for(m = 0; m < Region[k][l].size ; m ++ )      // release those adresses and update variables
                                {
                                    if(region_available[k] > 0)                    // if no more available and we havent reached the amount yet, this means its overlapping into the next region
                                    {
                                        region_available[k]--;                     // more regions available
                                        subregionsV2 |= (1ULL << (m+offset)) ;                         // these subregions are now available
                                                                          // moving to the next region
                                    }
                                    else
                                    {
                                        if(!offsetUpdated)                          // only update the offset once
                                        {
                                            offset += 8;
                                            offsetUpdated = true;
                                        }

                                        region_available[k+1]--;                                // finish freeing up the memory
                                        subregionsV2 |= (1ULL << (m+offset)) ;
                                    }

                                }
                                putsUart0("Heap Memory @ ");
                                num_to_HEX((uint32_t)Region[k][l].heap_address,hex_display);
                                putsUart0(hex_display);
                                putsUart0(" size of ");
                                num_to_HEX(Region[k][l].size *  Region[k][l].type , hex_display);
                                putsUart0(hex_display);
                                putsUart0(" is now free...\n");

                                assignMemory(Region,k,l,ZERO,ZERO,ZERO,NA);

                            }

                        }
                    }

                    return;
                }
            }

        }

        putsUart0("Sorry can't do that...\n");               // memory either not in reigon
                                                           // or dont have permision
                                                           // have to come back to check on this functionality

}

void assignMemory(mem_addr myRegion[5][8], uint8_t region, uint8_t subregion,  uint32_t myPID, uint8_t mySIZE, mem_type type, void *myAddress )
{

        myRegion [region][subregion].PID = myPID;
        myRegion [region][subregion].size = mySIZE;
        myRegion [region][subregion].type = type;
        myRegion [region][subregion].heap_address = myAddress;
        myRegion [region][subregion].topOfStack = myAddress + (mySIZE * type);


}

uint8_t find_open_memory(uint8_t region)
{
    uint8_t i,limit;
    uint8_t open_slot = 0;
    switch (region)                     // make sure the loop check those specific regions
    {
      case 0:
                  i = 0;
                  limit = i + 8;
                  break;
      case 1:
                  i = 8;
                  limit = i + 8;
                  break;
      case 2:
                  i = 16;
                  limit = i + 8;
                  break;
      case 3:
                  i = 24;
                  limit = i + 8;
                  break;
      case 4:
                  i = 32;
                  limit = i + 8;
                  break;

    }

        for(i ; i < limit; i ++)            // see if these sub regions are good
           {
              if(subregionsV2 & (1 << i) )
              {
                  subregionsV2 &= ~(1ULL << i) ;                         // this subregion is no longer available to be used
                  region_available[region]++;                           // FILL UP CAPACITY
                  //putsUart0("MEMORY ALLOCATED\n");
                  return open_slot;
              }
              open_slot++;

           }

    return 50;
}

uint8_t check_contiguous_memory(uint8_t region, uint8_t block_amount )
{
    uint8_t i= 0;
    uint8_t j= 0;
    uint8_t k= 0;
    uint8_t limit= 0;
    uint8_t check = 1;
    uint8_t base_position= 0;
    uint64_t stationary,moved;

    switch (region)                     // make sure the loop check those specific regions
    {
      case 0:
                  i = 0;
                  limit = i + 8;
                  break;
      case 1:
                  i = 8;
                  limit = i + 8;
                  break;
      case 2:
                  i = 16;
                  limit = i + 8;
                  break;
      case 3:
                  i = 24;
                  limit = i + 8;
                  break;
      case 4:
                  i = 32;
                  limit = i + 8;
                  break;

    }

    for( i ; i < limit; i ++)
    {   stationary = 1ULL << i;
        stationary= (stationary & subregionsV2) ? 1 : 0;
        if(stationary == 0)
        {
            base_position ++;
            continue;
        }
        for(j=i+1 ; j < limit ; j ++)
        {
            moved = 1ULL << j;
            moved= (moved & subregionsV2) ? 1 : 0;

            if( (stationary == 1) && (moved == 1 ) ) check++;
            if(check == block_amount)                                               // block amount obtained
            {
                for(k = i; k < i + block_amount ; k ++)
                {
                    subregionsV2 &= ~(1ULL << k) ;                         // this subregion is no longer available to be used
                }
                region_available[region] += block_amount;
                //putsUart0("MEMORY ALLOCATED\n");
                return base_position ;
            }
            else if ( (stationary == 1) && (moved == 0 ) ) break;
            else if ( (stationary == 0) && (moved == 1 ) ) break;
        }
        base_position ++;
        check = 1;
    }

    putsUart0("Sorry...No contiguous memory available to fit that amount rn \n");

    return 50;
}

uint8_t check_contiguous_memory512(uint8_t region, uint8_t block_amount )
{
    uint8_t i= 0;
    uint8_t j= 0;
    uint8_t k= 0;
    uint8_t limit= 0;
    uint8_t check = 1;
    uint8_t base_position= 0;
    uint64_t stationary,moved;
    switch (region)                     // make sure the loop check those specific regions
    {
      case 0:
                  i = 0;
                  limit = i + 8;
                  break;
      case 2:                                                   // this will run from the bottom of Region2 to the top of Region3
                  i = 16;
                  limit = i + 16;
                  break;

    }

    for( i ; i < limit; i ++)
    {   stationary = 1ULL << i;
        stationary= (stationary & subregionsV2) ? 1 : 0;
        if(stationary == 0)
        {
            base_position ++;
            continue;
        }
        for(j=i+1 ; j < limit ; j ++)
        {
            moved = 1ULL << j;
            moved= (moved & subregionsV2) ? 1 : 0;

            if( (stationary == 1) && (moved == 1 ) ) check++;
            if(check == block_amount)                                               // block amount obtained
            {
                for(k = i; k < i + block_amount ; k ++)
                {
                    subregionsV2 &= ~(1ULL << k) ;                         // this subregion is no longer available to be used

                    if(region_available[region] == 8)
                    {
                        region_available[region + 1]++;                    // move to the next region if there are not enough spots available.
                    }
                    else region_available[region]++;

                }

                //putsUart0("MEMORY ALLOCATED\n");
                return base_position ;
            }
            else if ( (stationary == 1) && (moved == 0 ) ) break;
            else if ( (stationary == 0) && (moved == 1 ) ) break;
        }
        base_position ++;
        check = 1;
    }

    putsUart0("Sorry...No contiguous memory available to fit that amount\n");
    return 50;
}

// DONE: include your solution from the mini project
void allowFlashAccess(void)
{
    // Flash memory setup
       NVIC_MPU_BASE_R = NVIC_MPU_BASE_VALID | FLASH_BASE | REGION_7;                      // base address updatied from region field in this register
       NVIC_MPU_ATTR_R = FLASH_MEMORY_SETUP  | FLASH_MEMORY_SIZE | RW_BOTH | ENABLE_REGION ;
}

void allowPeripheralAccess(void)
{
        // Peripherals Setup
        NVIC_MPU_BASE_R = NVIC_MPU_BASE_VALID | PERIPHERAL_BASE |  REGION_1;                      // base address updated from region field in this register
        NVIC_MPU_ATTR_R = PERIPHERALS_SETUP   | PERIPHERAL_MEMORY_SIZE | RW_BOTH | ENABLE_REGION ;
}

void setupSramAccess(void)
{
    //    Memory layout
    //        8k
    //        4k
    //        4k
    //        8k
    //        4k
    //        OS
    NVIC_MPU_BASE_R = NVIC_MPU_BASE_VALID | REGION4_BASE | REGION_6;                      // base address updated from region field in this register
    NVIC_MPU_ATTR_R = ALL_SRAM_SETUP | SIZE_8K | RW_BOTH | ENABLE_REGION ;

    NVIC_MPU_BASE_R = NVIC_MPU_BASE_VALID | REGION3_BASE | REGION_5;                      // base address updated from region field in this register
    NVIC_MPU_ATTR_R = ALL_SRAM_SETUP | SIZE_4K | RW_BOTH | ENABLE_REGION ;

    NVIC_MPU_BASE_R = NVIC_MPU_BASE_VALID | REGION2_BASE | REGION_4;                      // base address updated from region field in this register
    NVIC_MPU_ATTR_R = ALL_SRAM_SETUP  | SIZE_4K | RW_BOTH | ENABLE_REGION ;

    NVIC_MPU_BASE_R = NVIC_MPU_BASE_VALID | REGION1_BASE | REGION_3;                      // base address updated from region field in this register
    NVIC_MPU_ATTR_R = ALL_SRAM_SETUP  | SIZE_8K | RW_BOTH | ENABLE_REGION ;

    NVIC_MPU_BASE_R = NVIC_MPU_BASE_VALID | REGION0_BASE | REGION_2;                      // base address updated from region field in this register
    NVIC_MPU_ATTR_R = ALL_SRAM_SETUP  | SIZE_4K | RW_BOTH | ENABLE_REGION ;



}

uint64_t createNoSramAccessMask(void)
{
    // just returns the value to disable all subregions
      return ALL_SUBREGIONS_DISABLE;
}

void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes)
{
        uint8_t  region = 0;
        uint32_t  check_region = 0;
        uint8_t  subregion_start = 0;
        uint32_t subregions_to_enable = 0;
        uint32_t i;
        static const uint32_t region_masks[8][14] = {                                                                          // only 14 because the forced chunk will need 2 spaces for memory
            { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF},                     // REGION_0_ENABLE to REGION_0to14_ENABLE
            { 0x02, 0x06, 0x0E, 0x1E, 0x3E, 0x7E, 0xFE, 0x1FE, 0x3FE, 0x7FE, 0xFFE, 0x1FFE, 0x3FFE, 0x7FFE },                  // REGION_1_ENABLE to REGION_1to14_ENABLE
            { 0x04, 0x0C, 0x1C, 0x3C, 0x7C, 0xFC, 0x1FC, 0x3FC, 0x7FC, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFF00 },                // REGION_2_ENABLE to REGION_2to14_ENABLE
            { 0x08, 0x18, 0x38, 0x78, 0xF8, 0x1F8, 0x3F8, 0x7F8, 0xFF8, 0x1FF8, 0x3FF8, 0x7FF8, 0xFF00, 0x1FF00},              // REGION_3_ENABLE to REGION_3to14_ENABLE
            { 0x10, 0x30, 0x70, 0xF0, 0x1F0, 0x3F0, 0x7F0, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFF00, 0x1FF00, 0x3FF00},           // REGION_4_ENABLE to REGION_4to14_ENABLE
            { 0x20, 0x60, 0xE0, 0x1E0, 0x3E0, 0x7E0, 0xFE0, 0x1FE0, 0x3FE0, 0x7FE0, 0xFFE0, 0x1FFE0, 0x3FFE0, 0x7FFE0},        // REGION_5_ENABLE to REGION_5to14_ENABLE
            { 0x40, 0xC0, 0x1C0, 0x3C0, 0x7C0, 0xFC0, 0x1FC0, 0x3FC0, 0x7FC0, 0xFF80, 0x1FF80, 0x3FF80, 0x7FF80, 0xFF800},     // REGION_6_ENABLE to REGION_6to14_ENABLE
            { 0x80, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000, 0x10000, 0x20000, 0x40000, 0x80000, 0x100000}  // REGION_7_ENABLE to REGION_7to14_ENABLE
        };

        switch ((uint32_t)baseAdd)
        {

          case REGION0_BASE ... (REGION1_BASE - 1) : region = 0;

                                          for( i = 0; i < 8; i++)                               // check the base address
                                          {
                                              check_region = REGION0_BASE + (i*512);
                                              if((uint32_t)baseAdd == check_region ) break;     // we have the correct address to start from
                                              subregion_start++;
                                          }
                                          subregions_to_enable = size_in_bytes / 512;

                                          // base address updated from region field in this register

                                          break;

          case REGION1_BASE ... (REGION2_BASE - 1) :region = 1;

                                          for( i = 0; i < 8; i++)                               // check the base address
                                          {
                                              check_region = REGION1_BASE + (i*1024);
                                              if((uint32_t)baseAdd == check_region ) break;     // we have the correct address to start from
                                              subregion_start++;
                                          }
                                          subregions_to_enable = size_in_bytes / 1024;

                                          // base address updated from region field in this register
                                          break;

          case REGION2_BASE ... (REGION3_BASE - 1) :region = 2;

                                          for( i = 0; i < 8; i++)                               // check the base address
                                          {
                                              check_region = REGION2_BASE + (i*512);
                                              if((uint32_t)baseAdd == check_region ) break;     // we have the correct address to start from
                                              subregion_start++;
                                          }
                                          subregions_to_enable = size_in_bytes / 512;

                                          // base address updated from region field in this register
                                          break;

          case REGION3_BASE ... (REGION4_BASE - 1) :region = 3;

                                          for( i = 0; i < 8; i++)                               // check the base address
                                          {
                                              check_region = REGION3_BASE + (i*512);
                                              if((uint32_t)baseAdd == check_region ) break;     // we have the correct address to start from
                                              subregion_start++;
                                          }
                                          subregions_to_enable = size_in_bytes / 512;

                                          // base address updated from region field in this register
                                          break;

          case REGION4_BASE ... (REGION_MAX - 1)   :region = 4;
                                            for( i = 0; i < 8; i++)                       // check the base address
                                            {
                                                check_region = REGION4_BASE + (i*1024);
                                                  if((uint32_t)baseAdd == check_region )    // we have the correct address to start from
                                                  {
                                                      break;
                                                  }
                                                subregion_start++;
                                            }
                                            subregions_to_enable = size_in_bytes / 1024;
                                            // base address updated from region field in this register

                                            break;


        }

        if ( region >= 0 && region <= 4)
        {
            switch(subregion_start)
            {
                case 0:
                if (subregion_start >= 0 && subregion_start <= 7)
                {
                    if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                    {
                        *srdBitMask &= ~(region_masks[0][subregions_to_enable-1] << (region * 8));
                    }
                }
                break;

                case 1:
                    if (subregion_start >= 0 && subregion_start <= 7)
                    {
                        if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                        {
                            *srdBitMask &= ~(region_masks[1][subregions_to_enable-1] << (region * 8));
                        }
                    }
                    break;
                case 2:
                    if (subregion_start >= 0 && subregion_start <= 7)
                    {
                        if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                        {
                            *srdBitMask &= ~(region_masks[2][subregions_to_enable-1] << (region * 8));
                        }
                    }
                    break;
                case 3:
                    if (subregion_start >= 0 && subregion_start <= 7)
                    {
                        if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                        {
                            *srdBitMask &= ~(region_masks[3][subregions_to_enable-1] << (region * 8));
                        }
                    }
                    break;

                case 4:
                    if (subregion_start >= 0 && subregion_start <= 7)
                    {
                        if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                        {
                            *srdBitMask &= ~(region_masks[4][subregions_to_enable-1] << (region * 8));
                        }
                    }
                    break;
                case 5:
                    if (subregion_start >= 0 && subregion_start <= 7)
                    {
                        if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                        {
                            *srdBitMask &= ~(region_masks[5][subregions_to_enable-1] << (region * 8));
                        }
                    }
                    break;
                case 6:
                    if (subregion_start >= 0 && subregion_start <= 7)
                    {
                        if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                        {
                            *srdBitMask &= ~(region_masks[6][subregions_to_enable-1] << (region * 8));
                        }
                    }
                    break;

                case 7:
                    if (subregion_start >= 0 && subregion_start <= 7)
                    {
                        if (subregions_to_enable >= 1 && subregions_to_enable <= 14)
                        {
                            *srdBitMask &= ~(region_masks[7][subregions_to_enable-1] << (region * 8));
                        }
                    }
                    break;
            }
        }
}


void applySramAccessMask(uint64_t srdBitMask)
{
        int region = 0;

        for(region = 2; region < 7 ; region ++)
        {
            NVIC_MPU_NUMBER_R = region;

            switch (region)
                    {

                        case 2:

                                            NVIC_MPU_ATTR_R = ALL_SRAM_SETUP | SIZE_4K | RW_BOTH | ENABLE_REGION | (srdBitMask & 0x00000000FF)<< 8;

                                           break;
                        case 3:

                                            NVIC_MPU_ATTR_R = ALL_SRAM_SETUP  | SIZE_8K | RW_BOTH | ENABLE_REGION | (srdBitMask & 0x000000FF00) ;

                                           break;
                        case 4:

                                            NVIC_MPU_ATTR_R = ALL_SRAM_SETUP  | SIZE_4K | RW_BOTH | ENABLE_REGION | (srdBitMask & 0x0000FF0000) >> 8;

                                           break;
                        case 5:

                                            NVIC_MPU_ATTR_R = ALL_SRAM_SETUP  | SIZE_4K | RW_BOTH | ENABLE_REGION | (srdBitMask & 0x00FF000000) >>16;

                                           break;

                        case 6:
                                            NVIC_MPU_ATTR_R = ALL_SRAM_SETUP  | SIZE_8K | RW_BOTH | ENABLE_REGION | (srdBitMask & 0xFF00000000 )>>24;

                                           break;
                    }

        }

}

void enableBackgroundRegion()
{
    NVIC_MPU_CTRL_R |= NVIC_MPU_CTRL_PRIVDEFEN | NVIC_MPU_CTRL_ENABLE;
}

void enableFaultExceptions(void)
{
    // enable faults                MPU                       BUS                 USAGE
    NVIC_SYS_HND_CTRL_R  |= NVIC_SYS_HND_CTRL_MEM | NVIC_SYS_HND_CTRL_BUS | NVIC_SYS_HND_CTRL_USAGE;


}

uint32_t do_i_have_extra_memory(uint32_t myPID)
{

    uint8_t k,l;
    uint8_t anotherRegionFound = 0;



    for(k = 0; k < 5 ; k++)
    {

        for(l = 0; l < 8 ; l++)
        {
            if(Region[k][l].PID == myPID)
            {
                  anotherRegionFound ++;
                  if(anotherRegionFound == 2) return Region[k][l].heap_address;

            }

        }

    }


    return 0;


}

uint32_t whatsTheSizeOfThatExtraMemory(uint32_t myPID)
{
    uint8_t k,l;
    uint8_t anotherRegionFound = 0;



    for(k = 0; k < 5 ; k++)
    {

        for(l = 0; l < 8 ; l++)
        {
            if(Region[k][l].PID == myPID)
            {
                  anotherRegionFound ++;
                  if(anotherRegionFound == 2) return Region[k][l].size * Region[k][l].type;

            }

        }

    }

    return 0;


}
