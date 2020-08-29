
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "Dev_Inf.h"
#include "dqspi.h"

#define DSPI_START_ADDR_MAP          0x90000000


int main(void);
void SystemInit(void);


/**
  * Description :
  * Write data to the device
  * Inputs    :
  *      Address  : Write location
  *      Size     : Length in bytes  
  *      buffer   : Address where to get the data to write
  * outputs   :
  *      R0       : "1" 	: Operation succeeded	
  * 		    "0" 	: Operation failure
  * Info :
  * Align and memory size (32/16/8 bits) is handled in this function 
  * Note : Mandatory for all types except SRAM and PSRAM	
  */
int Write(uint32_t Address, uint32_t Size, uint32_t Buffer)
{
	DQSpiReset();

    if (DQSpiWrite(Address-DSPI_START_ADDR_MAP, (unsigned char *)Buffer, Size) != 0) {
        return 0;
    }

	DQSpiReset();
	DQSpiMemoryMapped();

    return 1;
}

/*******************************************************************************
 Description :
 Erase a full sector in the device
 Inputs :
 				SectrorAddress	: Start of sector
 outputs :
 				"1" : Operation succeeded
 				"0" : Operation failure
 Note : Not Mandatory for SRAM PSRAM and NOR_FLASH
********************************************************************************/
int SectorErase(uint32_t EraseStartAddress, uint32_t EraseEndAddress)
{
	uint32_t sct_start, sect_size, sct_end, i;

	DQSpiFlashInfo(NULL, &sect_size, NULL, NULL);

	DQSpiReset();

    sct_start = (EraseStartAddress-DSPI_START_ADDR_MAP) / sect_size;
    sct_end = (EraseEndAddress-DSPI_START_ADDR_MAP) / sect_size + 1;

    if (sct_start > sct_end)
    	sct_end = sct_start + 1;

    for (i=sct_start; i!=sct_end; i++) {
        if (DQSpiEraseBlock(i*sect_size) != 0) {
            return 0;
        }
    }

	DQSpiReset();
	DQSpiMemoryMapped();

    return 1;
}


int MassErase(void)
{
	DQSpiReset();

	if (DQSpiEraseChip() == 0)
		return 1;

	DQSpiReset();
	DQSpiMemoryMapped();

	return 0;
}


/**
  * Description :
  * Initilize the MCU Clock, the GPIO Pins corresponding to the
  * device and initilize the FSMC with the chosen configuration
  * Inputs    :
  *      None
  * outputs   :
  *      R0       : "1" 	: Operation succeeded
  * 		    "0" 	: Operation failure
  * Info :
  * Align and memory size (32/16/8 bits) is handled in this function
  * Note : Mandatory for all types except SRAM and PSRAM
  */
int Init(void)
{
    int ret;

    SystemInit();

	ret = main();

	if (DQSpiReset() != 0) {
		ret = 0;
	}

	DQSpiMemoryMapped();

    return ret;
}


/*******************************************************************************
Description :		       Initialisation of ticks counter
Inputs :
TickPriority : Set tick priority
outputs :
NONE
********************************************************************************/
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
  return HAL_OK;
}


/*******************************************************************************
Description :		       Delay in mS.
Inputs :
Delay : Time in mS
outputs :
NONE
********************************************************************************/
volatile uint32_t forse_time;
void HAL_Delay(__IO uint32_t Delay)
{
	volatile uint32_t i, j;

	for(i=0; i!=(Delay); i++) {
		for (j=0; j!=216000; j++) {
			forse_time++;
		}
	}
}
