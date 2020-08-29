
#include "main.h"

#include "dqspi.h"

// W25Q32FV winbond

#define QSPI_PAGE_SIZE                       256

/* Reset Operations */
#define RESET_ENABLE_CMD                     0x66
#define RESET_MEMORY_CMD                     0x99

/* Identification Operations */
#define READ_ID_CMD                          0x9E
#define READ_SERIAL_FLASH_DISCO_PARAM_CMD    0x5A

/* Read Operations */
#define READ_CMD                             0x03
#define FAST_READ_CMD                        0x0B
#define BURST_READ_WAP_CMD                   0x0C

#define DUAL_OUT_FAST_READ_CMD               0x3B
#define DUAL_OUT_FAST_READ_DTR_CMD           0x3D

#define DUAL_INOUT_FAST_READ_CMD             0xBB

#define QUAD_OUT_FAST_READ_CMD               0x6B

#define QUAD_INOUT_FAST_READ_CMD             0xEB

#define JEDEC_ID_CMD                         0x9F

/* Write Operations */
#define WRITE_ENABLE_CMD                     0x06
#define WRITE_DISABLE_CMD                    0x04

/* Register Operations */
#define READ_STATUS_REG1_CMD                 0x05
#define WRITE_STATUS_REG1_CMD                0x01
#define READ_STATUS_REG2_CMD                 0x35
#define WRITE_STATUS_REG2_CMD                0x31
#define READ_STATUS_REG3_CMD                 0x15
#define WRITE_STATUS_REG3_CMD                0x11

#define WRITE_ENABLE_STATUS_REG_CMD          0x50

/* Program Operations */
#define PAGE_PROG_CMD                        0x02

#define QUAD_IN_FAST_PROG_CMD                0x32

/* Erase Operations */
#define SECTOR_ERASE_CMD                     0x20

#define BLOCK_ERASE_CMD                      0x52 // 32k block

#define CHIP_ERASE_CMD                       0xC7 // 0x60

#define PROG_ERASE_RESUME_CMD                0x7A
#define PROG_ERASE_SUSPEND_CMD               0x75

/* One-Time Programmable Operations */
#define READ_UIC_ID_CMD                      0x4B
#define PROG_SECURITY_REG_CMD                0x42

/* Default dummy clocks cycles */
#define DUMMY_CLOCK_CYCLES_READ              8
#define DUMMY_CLOCK_CYCLES_READ_QUAD         10

#define DUMMY_CLOCK_CYCLES_READ_DTR          6
#define DUMMY_CLOCK_CYCLES_READ_QUAD_DTR     8



/* Flag Status Register */
#define W25Q32FV_FSR_BUSY                    ((uint8_t)0x01)    /*!< busy */
#define W25Q32FV_FSR_WREN                    ((uint8_t)0x02)    /*!< write enable */
#define W25Q32FV_FSR_QE                      ((uint8_t)0x02)    /*!< quad enable */

/* altternate bytes */
#define W25Q32FV_ALTERNATE_BYTE_M            0xFF

/* flash info */
#define W25Q32FV_FLASH_SIZE                  0x00400000UL // 32Mbit =>4Mbyte
#define W25Q32FV_SECTOR_SIZE                 0x00001000UL // 4K
#define W25Q32FV_BLOCK_SIZE                  0x00008000UL // 32K
#define W25Q32FV_PAGE_SIZE                   0x00000100UL // 256 bytes


extern QSPI_HandleTypeDef hqspi;


static int8_t DQSpiWriteEnable(void)
{
	QSPI_CommandTypeDef sCommand = {0};
	QSPI_AutoPollingTypeDef sConfig = {0};

	/* Enable write operations ------------------------------------------ */
	sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction = WRITE_ENABLE_CMD;
	sCommand.AddressMode = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode = QSPI_DATA_NONE;
	sCommand.DummyCycles = 0;
	sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	if (HAL_QSPI_Command(&hqspi, &sCommand, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
	  return -1;
	}

	/* Configure automatic polling mode to wait for write enabling ---- */
	sConfig.Match = W25Q32FV_FSR_WREN;
	sConfig.Mask = W25Q32FV_FSR_WREN;
	sConfig.MatchMode = QSPI_MATCH_MODE_AND;
	sConfig.StatusBytesSize = 1;
	sConfig.Interval = 0x10;
	sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

	sCommand.Instruction = READ_STATUS_REG1_CMD;
	sCommand.DataMode = QSPI_DATA_1_LINE;

	if (HAL_QSPI_AutoPolling(&hqspi, &sCommand, &sConfig, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
	  return -1;
	}

	return 0;
}


static uint8_t DQSpiAutoPollingMemReady(void)
{
	QSPI_CommandTypeDef sCommand = {0};
	QSPI_AutoPollingTypeDef sConfig = {0};

	/* Configure automatic polling mode to wait for memory ready ------ */
	sCommand.InstructionMode = QSPI_INSTRUCTION_1_LINE;
	sCommand.Instruction = READ_STATUS_REG1_CMD;
	sCommand.AddressMode = QSPI_ADDRESS_NONE;
	sCommand.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
	sCommand.DataMode = QSPI_DATA_1_LINE;
	sCommand.DummyCycles = 0;
	sCommand.DdrMode = QSPI_DDR_MODE_DISABLE;
	sCommand.DdrHoldHalfCycle  = QSPI_DDR_HHC_ANALOG_DELAY;
	sCommand.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

	sConfig.Match = 0x00;
	sConfig.Mask = W25Q32FV_FSR_BUSY;
	sConfig.MatchMode = QSPI_MATCH_MODE_AND;
	sConfig.StatusBytesSize = 1;
	sConfig.Interval = 0x10;
	sConfig.AutomaticStop = QSPI_AUTOMATIC_STOP_ENABLE;

	if (HAL_QSPI_AutoPolling(&hqspi, &sCommand, &sConfig, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -1;
	}

	return 0;
}


static uint8_t DQSpiResetMemory(void)
{
    QSPI_CommandTypeDef s_command = {0};

    /* Initialize the reset enable command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = RESET_ENABLE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Send the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    /* Send the reset memory command */
    s_command.Instruction = RESET_MEMORY_CMD;
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    /* Configure automatic polling mode to wait the memory is ready */
    if (DQSpiAutoPollingMemReady() != 0) {
        return -1;
    }

    return 0;
}


int8_t DQSpiReset(void)
{
	// deinit HAL
    if (HAL_QSPI_DeInit(&hqspi) !=  HAL_OK) {
        return -1;
    }

    // init HAL
    if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
        return -1;
    }

    /* QSPI memory reset */
    if (DQSpiResetMemory() != 0) {
        return -1;
    }

    return 0;
}


int8_t DQSpiFlashId(uint8_t *mid, uint16_t *id)
{
    QSPI_CommandTypeDef s_command = {0};
    uint8_t dat[3];

    /* Initialize the read command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = JEDEC_ID_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.AlternateBytesSize = 0;
    s_command.AlternateBytes = 0;
    s_command.DummyCycles = 0;
    s_command.NbData = 3;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;


    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(&hqspi, dat, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    *mid = dat[0];
    *id = dat[1];
    *id = ((*id)<<8)|dat[2];

    return 0;
}


int8_t DQSpiFlashInfo(uint32_t *blk_num, uint32_t *blk_size, uint32_t *sect_mum, uint32_t *sect_size)
{

	if (blk_num != NULL) {
		*blk_num = W25Q32FV_FLASH_SIZE/W25Q32FV_BLOCK_SIZE;
	}

	if (blk_size != NULL) {
		*blk_size = W25Q32FV_BLOCK_SIZE;
	}

	if (sect_mum != NULL) {
		*sect_mum = W25Q32FV_FLASH_SIZE/W25Q32FV_SECTOR_SIZE;
	}

	if (sect_size != NULL) {
		*sect_size = W25Q32FV_SECTOR_SIZE;
	}

	return 0;
}


int8_t DQSpiEraseChip(void)
{
    QSPI_CommandTypeDef s_command = {0};

    /* Initialize the erase command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = CHIP_ERASE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_NONE;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Enable write operations */
    if (DQSpiWriteEnable() != 0) {
        return -1;
    }

    /* Send the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    /* Configure automatic polling mode to wait for end of erase */
    if (DQSpiAutoPollingMemReady() != 0) {
        return -1;
    }

    return 0;
}


int8_t DQSpiEraseBlock(uint32_t addr)
{
    QSPI_CommandTypeDef s_command = {0};

    /* Initialize the erase command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = BLOCK_ERASE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = addr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Enable write operations */
    if (DQSpiWriteEnable() != 0) {
        return -1;
    }

    /* Send the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    /* Configure automatic polling mode to wait for end of erase */
    if (DQSpiAutoPollingMemReady() != 0) {
        return -1;
    }

    return 0;
}


int8_t DQSpiEraseSector(uint32_t addr)
{
    QSPI_CommandTypeDef s_command = {0};

    /* Initialize the erase command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = SECTOR_ERASE_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = addr;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DataMode = QSPI_DATA_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Enable write operations */
    if (DQSpiWriteEnable() != 0) {
        return -1;
    }

    /* Send the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    /* Configure automatic polling mode to wait for end of erase */
    if (DQSpiAutoPollingMemReady() != 0) {
        return -1;
    }

    return 0;
}


int8_t DQSpiRead(uint32_t addr, uint8_t *dat, uint32_t len)
{
    QSPI_CommandTypeDef s_command = {0};

    /* Initialize the read command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = DUAL_OUT_FAST_READ_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.Address = addr;
    s_command.DataMode = QSPI_DATA_2_LINES;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.AlternateBytesSize = 0;
    s_command.AlternateBytes = 0;
    s_command.DummyCycles = DUMMY_CLOCK_CYCLES_READ;
    s_command.NbData = len;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;


    /* Configure the command */
    if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    /* Reception of the data */
    if (HAL_QSPI_Receive(&hqspi, dat, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
        return -1;
    }

    return 0;
}


int8_t DQSpiWrite(uint32_t addr, uint8_t *dat, uint32_t len)
{
    QSPI_CommandTypeDef s_command = {0};
    uint32_t end_addr, current_size, current_addr;

    if (len == 0)
    	return 0;

    /* Calculation of the size between the write address and the end of the page */
    current_size = W25Q32FV_PAGE_SIZE - (addr % W25Q32FV_PAGE_SIZE);

    /* Check if the size of the data is less than the remaining place in the page */
    if (current_size > len) {
        current_size = len;
    }

    /* Initialize the adress variables */
    current_addr = addr;
    end_addr = addr + len;

    /* Initialize the program command */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = PAGE_PROG_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.DataMode = QSPI_DATA_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.DummyCycles = 0;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Perform the write page by page */
    do {
        s_command.Address = current_addr;
        s_command.NbData = current_size;

    	/* Enable write operations */
    	if (DQSpiWriteEnable() != 0) {
    		return -1;
    	}

        /* Configure the command */
        if (HAL_QSPI_Command(&hqspi, &s_command, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return -1;
        }

        /* Transmission of the data */
        if (HAL_QSPI_Transmit(&hqspi, dat, HAL_QPSI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
            return -1;
        }

        /* Configure automatic polling mode to wait for end of program */
        if (DQSpiAutoPollingMemReady() != 0) {
            return -1;
        }

        /* Update the address and size variables for next page programming */
        current_addr += current_size;
        dat += current_size;
        current_size = ((current_addr + W25Q32FV_PAGE_SIZE) > end_addr) ? (end_addr - current_addr) : W25Q32FV_PAGE_SIZE;
    } while (current_addr < end_addr);

    return 0;
}


int8_t DQSpiMemoryMapped(void)
{
    QSPI_CommandTypeDef s_command = {0};
    QSPI_MemoryMappedTypeDef s_mem_mapped_cfg = {0};

    /* Configure the command for the read instruction */
    s_command.InstructionMode = QSPI_INSTRUCTION_1_LINE;
    s_command.Instruction = DUAL_OUT_FAST_READ_CMD;
    s_command.AddressMode = QSPI_ADDRESS_1_LINE;
    s_command.AddressSize = QSPI_ADDRESS_24_BITS;
    s_command.DataMode = QSPI_DATA_2_LINES;
    s_command.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
    s_command.AlternateBytesSize = 0;
    s_command.AlternateBytes = 0;
    s_command.DummyCycles = DUMMY_CLOCK_CYCLES_READ;
    s_command.DdrMode = QSPI_DDR_MODE_DISABLE;
    s_command.DdrHoldHalfCycle = QSPI_DDR_HHC_ANALOG_DELAY;
    s_command.SIOOMode = QSPI_SIOO_INST_EVERY_CMD;

    /* Configure the memory mapped mode */
    s_mem_mapped_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    s_mem_mapped_cfg.TimeOutPeriod = 0;

    if (HAL_QSPI_MemoryMapped(&hqspi, &s_command, &s_mem_mapped_cfg) != HAL_OK) {
        return -1;;
    }

    return 0;
}


