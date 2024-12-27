// Memory manager functions
// DeZean Gardner

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

#ifndef MM_H_
#define MM_H_
//    Memory layout
//        8k
//        4k
//        4k
//        8k
//        4k
//        OS
//-----------------------------------------------------------------------------
// MACROS
//-----------------------------------------------------------------------------
#define NUM_SRAM_REGIONS 4

#define BASIC_MEMORY  512
#define CHUNK_MEMORY  1024

#define ZERO    0
#define ONE     1
#define TWO     2
#define THREE   3
#define FOUR    4
#define FIVE    5
#define SIX     6
#define SEVEN   7
#define EIGHT   8
#define NINE    9
#define TEN     10
#define ELEVEN  11
#define TWELVE  12
#define THIRTEEN  13
#define FOURTEEN  14
#define FIFTEEN   15
#define SIXTEEN   16
#define MAX_FIT 8

// Memory access bits

#define REGION_MAX         0x20008000
#define REGION4_BASE       0x20006000       // 8K
#define REGION3_BASE       0x20005000       // 4K
#define REGION2_BASE       0x20004000       // 4K
#define REGION1_BASE       0x20002000       // 8K
#define REGION0_BASE       0x20001000       // 4K

                                            // NA : no access
                                            // RW : read and write
                                            // RO : read only
#define NA_BOTH             0x00000000
#define RW_PRIV_NA_UNPRIV   0x01000000
#define RW_PRIV_RO_UNPRIV   0x02000000
#define RW_BOTH             0x03000000
#define RO_PRIV_NA_UNPRIV   0x05000000
#define RO_BOTH             0x07000000

#define PERIPHERAL_BASE                      0x40000000
#define PRIVATE_PERIPHERAL_BASE              0xE0000000
#define BITBAND_PERIPHERAL_ALIAS_BASE        0x42000000
#define BITBAND_MEMORY_ALIAS_BASE            0x22000000
#define OPERATING_SYS_BASE                   0x20000000
#define SRAM_BASE                            0x20000000
#define FLASH_BASE                           0x00000000
#define ALL_MEMORY_BASE                      0x00000000
                                                                       // sizes
#define ALL_MEMORY_SIZE                             0x0000003E         // 4GB     OK
#define PERIPHERAL_MEMORY_SIZE                      (25 << 1)          // 64MB    OK
#define PRIVATE_PERIPHERAL_MEMORY_SIZE              0x00000036         // 268MB   OK
#define BITBAND_PERIPHERAL_ALIAS_MEMORY_SIZE        0x00000030         // 32MB    OK
#define BITBAND_MEMORY_ALIAS_MEMORY_SIZE            0x00000026         // 1MB     OK
#define OPERATING_SYS_MEMORY_SIZE                   0x00000016         // 4KB     OK
#define SRAM_MEMORY_SIZE                            0x0000001C         // 32KB    OK
#define FLASH_MEMORY_SIZE                           0x00000022         // 262KB   OK
#define SIZE_4K                                     0x00000016         // 4KB     OK
#define SIZE_8K                                     0x00000018         // 8KB     OK

#define REGION_0_ENABLE                0x01
#define REGION_0to1_ENABLE             0x03
#define REGION_0to2_ENABLE             0x07
#define REGION_0to3_ENABLE             0x0F
#define REGION_0to4_ENABLE             0x1F
#define REGION_0to5_ENABLE             0x3F
#define REGION_0to6_ENABLE             0x7F
#define REGION_0to7_ENABLE             0xFF
////////////////////////////////////////////////////////////
#define REGION_1_ENABLE                0x02
#define REGION_1to2_ENABLE             0x06
#define REGION_1to3_ENABLE             0x0E
#define REGION_1to4_ENABLE             0x11
#define REGION_1to5_ENABLE             0x3E
#define REGION_1to6_ENABLE             0x7E
#define REGION_1to7_ENABLE             0xFE
////////////////////////////////////////////////////////////
#define REGION_2_ENABLE                0x04
#define REGION_2to3_ENABLE             0x0C
#define REGION_2to4_ENABLE             0x1C
#define REGION_2to5_ENABLE             0x3C
#define REGION_2to6_ENABLE             0x7C
#define REGION_2to7_ENABLE             0xFC
////////////////////////////////////////////////////////////
#define REGION_3_ENABLE                0x08
#define REGION_3to4_ENABLE             0x18
#define REGION_3to5_ENABLE             0x38
#define REGION_3to6_ENABLE             0x78
#define REGION_3to7_ENABLE             0xF8
////////////////////////////////////////////////////////////
#define REGION_4_ENABLE                0x10
#define REGION_4to5_ENABLE             0x30
#define REGION_4to6_ENABLE             0x70
#define REGION_4to7_ENABLE             0x0F
////////////////////////////////////////////////////////////
#define REGION_5_ENABLE                0x20
#define REGION_5to6_ENABLE             0x60
#define REGION_5to7_ENABLE             0xE2
////////////////////////////////////////////////////////////
#define REGION_6_ENABLE                0x40
#define REGION_6to7_ENABLE             0xC0
////////////////////////////////////////////////////////////
#define REGION_7_ENABLE                0x80


#define ALL_SUBREGIONS_DISABLE         0xFFFFFFFFFF
////////////////////////////////////////////////////////////

#define NEVER_EXECUTE             0x10000000
#define ALLOW_EXECUTION           0x10000000
                                                      // TEX | S | C | B
#define FLASH_MEMORY_SETUP        0x00020000          // 000 | 0 | 1 | 0
#define INTERNAL_SRAM_SETUP       0x00060000          // 000 | 1 | 1 | 0
#define ALL_SRAM_SETUP            0x00060000          // 000 | 1 | 1 | 0
#define EXTERNAL_SRAM_SETUP       0x00070000          // 000 | 1 | 1 | 1
#define PERIPHERALS_SETUP         0x00050000          // 000 | 1 | 0 | 1

#define ENABLE_REGION             0x00000001

#define REGION_0             0x00000000
#define REGION_1             0x00000001
#define REGION_2             0x00000002
#define REGION_3             0x00000003
#define REGION_4             0x00000004
#define REGION_5             0x00000005
#define REGION_6             0x00000006
#define REGION_7             0x00000007

#define SPECIAL_TEST_CASE 0x20000000
#define NULL 0

//-----------------------------------------------------------------------------
// Structs & Enums
//-----------------------------------------------------------------------------
typedef enum
{
    NA = 0,
    BASIC = 512,
    CHUNK = 1024,
    FORCED_CHUNCK = 1024,

}mem_type;

typedef struct
{
   uint32_t PID;                        // this pid  will not be used for now. size lookup will be based on heap address(base)
   uint8_t size;
   mem_type type;
   void *heap_address;
   uint32_t *topOfStack;
   uint8_t extraMemory;
}mem_addr;
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void * mallocFromHeap(uint32_t size_in_bytes);
void freeToHeap(void *pMemory);
void assignMemory(mem_addr myRegion[5][8], uint8_t region, uint8_t subregion,  uint32_t myPID, uint8_t mySIZE, mem_type type, void *myAddress );
uint8_t find_open_memory(uint8_t region);
uint8_t check_contiguous_memory(uint8_t region, uint8_t block_amount);
uint8_t check_contiguous_memory512(uint8_t region, uint8_t block_amount);
uint32_t whatsMySizeFromBase(uint32_t *base_Adress);
uint32_t whatsMySizeFromStack(uint32_t *topOfStack);
void whatsMyName(char *oldName, const char *newName);

void allowFlashAccess(void);
void allowPeripheralAccess(void);
void setupSramAccess(void);
uint64_t createNoSramAccessMask(void);
void addSramAccessWindow(uint64_t *srdBitMask, uint32_t *baseAdd, uint32_t size_in_bytes);
void applySramAccessMask(uint64_t srdBitMask);

void enableBackgroundRegion();
void enableFaultExceptions(void);

uint32_t do_i_have_extra_memory(uint32_t myPID);
uint32_t whatsTheSizeOfThatExtraMemory(uint32_t myPID);

#endif
