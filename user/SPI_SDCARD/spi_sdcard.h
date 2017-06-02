

#ifndef __SPI_SDCARD_H
#define __SPI_SDCARD_H

#include <stm32f10x.h>
#include "diskio.h"

#define SPI_SD_SCK      		GPIO_Pin_3
#define SPI_SD_MISO     		GPIO_Pin_4
#define SPI_SD_MOSI     		GPIO_Pin_5

#define SPI_SD_CS       		GPIO_Pin_15
#define SPI_SD_CS_PORT			GPIOA

#define SPI_SD_Port     		GPIOB
#define SPI_SD          		SPI1

#define RCC_APBPeriph_SPI_SD  					RCC_APB2Periph_SPI1
#define RCC_APBPeriphClockCmd_SPI_SD  	RCC_APB2PeriphClockCmd

#define CardDetect_Pin				GPIO_Pin_6
#define CardDetect_Port				GPIOB
#define GPIO_IDR_IDR_Detect		GPIO_IDR_IDR6


/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define ACMD13	(0xC0+13)	/* SD_STATUS (SDC) */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT (MMC) */
#define ACMD23	(0xC0+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */

/* Card-Select Controls  (Platform dependent) */
#define SELECT()       GPIO_ResetBits(SPI_SD_CS_PORT, SPI_SD_CS)    /* MMC CS = L */
#define DESELECT()     GPIO_SetBits(SPI_SD_CS_PORT, SPI_SD_CS)      /* MMC CS = H */

extern volatile DSTATUS Stat;

typedef enum {
	SLOW, 
	FAST 
}SPEED;


typedef enum {
	TX,
	RX
}DIRECTION;

typedef struct{
	uint8_t 	csd_type; 
	uint16_t	sector_len;
	uint32_t	sector_count;
	uint32_t	card_capacity;
	uint16_t	erase_size;
	uint8_t 	erase_block_en;
	uint8_t 	read_current_min;
	uint8_t 	read_current_max;
	uint8_t 	write_current_min;
	uint8_t 	write_current_max;
	
}CSDTypeDef;

DSTATUS sd_card_init(void);
DRESULT SPI_SD_ReadBlock (uint32_t address,uint8_t *buff/*uint16_t blocksize*/);
DRESULT SPI_SD_ReadMultiBlock(uint32_t address,uint8_t *buff/*,uint16_t blocksize*/,uint32_t NumberOfBlocks);
DRESULT SPI_SD_WriteBlock (uint32_t address,const uint8_t *buff/*,uint16_t blocksize*/);
DRESULT SPI_SD_WriteMultiBlock(uint32_t address,const uint8_t *buff/*,uint16_t blocksize*/,uint32_t NumberOfBlocks);
void sdcard_reinitial(void);

void SPI_SD_GETCSD(void);
#endif /* __SPI_SDCARD_H */
