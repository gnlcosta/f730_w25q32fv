#include "Dev_Inf.h"

/* This structure containes information used by ST-LINK Utility to program and erase the device */
#if defined (__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
struct StorageInfo const StorageInfo = {
#endif
    .DeviceName = "W25Q32FV_STM32F730",       // Device Name + version number
	.DeviceType = SPI_FLASH,                  // Device Type
	.DeviceStartAddress = 0x90000000,         // Device Start Address
	.DeviceSize = 0x00400000,                 // Device Size in Bytes (32Mbits)
    .PageSize   = 0x00000100,                 // Programming Page Size
    .EraseValue = 0xFF,                       // Initial Content of Erased Memory
// Specify Size and Address of Sectors (view example below)
    .sectors = {
    		{  // Sector Num : 8 ,Sector Size: 32KBytes
    			.SectorNum = 0x00000080,
				.SectorSize = 0x000008000
    		},
			{
				.SectorNum = 0x00000000,
				.SectorSize = 0x00000000
			},
     },
};

/*  Sector coding example
    A device with succives 16 Sectors of 1KBytes, 128 Sectors of 16 KBytes, 
    8 Sectors of 2KBytes and 16384 Sectors of 8KBytes
	
    0x00000010, 0x00000400,     						// 16 Sectors of 1KBytes
    0x00000080, 0x00004000,     						// 128 Sectors of 16 KBytes
    0x00000008, 0x00000800,     						// 8 Sectors of 2KBytes
    0x00004000, 0x00002000,     						// 16384 Sectors of 8KBytes
    0x00000000, 0x00000000,							// end
 */
