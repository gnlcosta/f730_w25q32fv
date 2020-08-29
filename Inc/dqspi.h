

#ifndef __DQSPI_H__
#define __DQSPI_H__

#include <stdint.h>


#define FLASH_ID             0x4016
#define FLASH_MF_ID          0xEF


int8_t DQSpiReset(void);
int8_t DQSpiFlashId(uint8_t *mid, uint16_t *id);
int8_t DQSpiFlashInfo(uint32_t *blk_num, uint32_t *blk_size, uint32_t *sect_mum, uint32_t *sect_size);
int8_t DQSpiEraseChip(void);
int8_t DQSpiEraseBlock(uint32_t addr);
int8_t DQSpiEraseSector(uint32_t addr);
int8_t DQSpiRead(uint32_t addr, uint8_t *dat, uint32_t len);
int8_t DQSpiWrite(uint32_t addr, uint8_t *dat, uint32_t len);
int8_t DQSpiMemoryMapped(void);


#endif
