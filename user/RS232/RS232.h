
#ifndef __RS232_H
#define __RS232_H

#include <stm32f10x.h>

typedef struct{
	uint8_t Start;
	uint8_t DevAdr;
	uint8_t Cmd;
	uint8_t AdrL;
	uint8_t AdrH;
	uint8_t Length;
	uint8_t Crcl;
	uint8_t Crch;
	
}CMD7KT_TypeDef;
/*
*/
typedef struct{
	uint16_t Sn;
	uint8_t LengthArch;
	uint8_t VersArch;
	uint8_t VersDevice;
	uint8_t Year;
	uint8_t Month;
	uint8_t Day;
	uint8_t Hour;
	uint8_t Minute;
	uint8_t Sec;
}DEV7KT_TypeDef;	
/*
*/
typedef enum{
	S2400=4,
	S4800,
	S9600,
	S19200,
	S28800,
	S38400,
	S57600,
	S115200
}SPEED_RS232;
/*
*/
uint16_t check_global_crc(uint8_t *array,uint16_t number);
uint16_t  check_crc(uint8_t * array,uint16_t number);
uint8_t Init_connect_7KT(void);
void Сhange_speed_cmd(uint8_t speed);
uint8_t Read_current_data(uint8_t * buffer);
uint8_t Read_EEPROM(uint8_t * buffer,uint16_t AdrStart,uint16_t AdrStop);
uint8_t Read_currentData(uint8_t *buffer);
void hex2dec_byte(uint8_t* src,char *dst);
void hex2dec_word(uint16_t* src,char *dst);

void DMA_Usart_Init(void);
void ReadEEPROM_CmdDMA(uint8_t*buffer, uint16_t AdrStart);
void ReadRAM_CmdDMA(uint8_t *buffer,uint8_t vers_arch);
#endif
