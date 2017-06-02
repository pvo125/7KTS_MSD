#include <stm32f10x.h>
#include "RS232.h"
#include "LCD_Driver.h"
extern CMD7KT_TypeDef  cmd7kt_struct;
uint16_t crc;
volatile uint8_t tx7kt_transaction;
volatile uint8_t rx7kt_transaction;
extern uint16_t global_crc;
extern volatile uint8_t timeout_delay;
/*
*/
static void Add_GlobalCRC(uint8_t * byte){
	int i,Carry;
	uint8_t Byte=*byte;
	for(i=0;i<8;i++)
	{
		Carry=0;
		if(global_crc & 0x8000) Carry=1;
		global_crc<<=1;
		if(0x80 & Byte) global_crc |=0x01;
		Byte<<=1;
		if(Carry) global_crc^=0x1021;
	}
}
/*
*/
static void AddCRC(uint8_t * byte){
	int i,Carry;
	uint8_t Byte=*byte;
	for(i=0;i<8;i++)
	{
		Carry=0;
		if(crc & 0x8000) Carry=1;
		crc<<=1;
		if(0x80 & Byte) crc |=0x01;
		Byte<<=1;
		if(Carry) crc^=0x1021;
	}
}
/*
*/
uint16_t  check_crc(uint8_t * array,uint16_t number){
	uint16_t i;
	uint8_t  temp=0; 
	crc=1;
		for(i=0;i<number;i++)
			AddCRC(array+i);
		AddCRC(&temp);
		AddCRC(&temp);
		cmd7kt_struct.Crcl=(uint8_t)crc;
		cmd7kt_struct.Crch=crc>>8;
		return crc;
}
/*
*/
uint16_t check_global_crc(uint8_t *array,uint16_t number){
	uint16_t i;
	uint8_t  temp=0;
	
	for(i=0;i<number;i++)
		Add_GlobalCRC(array+i);
	if(number==8190)
	{
		Add_GlobalCRC(&temp);
		Add_GlobalCRC(&temp);
	}
	return global_crc;
}
/*
*/
void SendActiv_level(void){
		
		uint8_t buff[60]={0};
				
		DMA1_Channel4->CCR&=~DMA_CCR4_EN;  							  /* Disable DMA TX Channel */	
		DMA1_Channel4->CCR&=~DMA_CCR4_EN;  							  /* Disable DMA TX Channel */	
		
		DMA1_Channel4->CCR |=DMA_CCR4_TCIE;							// transfer complete interrupt enable  				
		DMA1_Channel5->CCR &=~DMA_CCR5_TCIE;						// transfer complete interrupt disable 	
		
		DMA1_Channel4->CMAR=(uint32_t)&buff;
		DMA1_Channel4->CNDTR=sizeof(buff);
				
		DMA1_Channel4->CCR|=DMA_CCR4_EN;  							/* Enable DMA TX Channel */	
		tx7kt_transaction=1;

}
/*
*/
void Change_speed_DMA(uint8_t * buffer,uint8_t speed){
		
		cmd7kt_struct.DevAdr=0;
		cmd7kt_struct.Cmd=6;
		cmd7kt_struct.AdrL=speed;
		cmd7kt_struct.AdrH=speed;
		cmd7kt_struct.Length=0;
		
		check_crc(&cmd7kt_struct.DevAdr,5);
		DMA1->IFCR|=DMA_IFCR_CTCIF4;//|DMA_IFCR_CTCIF5;		/*Clear TC4 TC5 flag */
	
		DMA1_Channel4->CCR&=~DMA_CCR4_EN;  							/* Disable DMA TX Channel */	
		DMA1_Channel5->CCR&=~DMA_CCR5_EN;  							/* Disable DMA RX Channel */	
		
		DMA1_Channel4->CMAR=(uint32_t)&cmd7kt_struct;	
		DMA1_Channel4->CNDTR=sizeof(cmd7kt_struct);
				
		DMA1_Channel5->CMAR=(uint32_t)buffer;
		DMA1_Channel5->CNDTR=1;
		DMA1_Channel5->CCR|=DMA_CCR5_TCIE;							// transfer complete interrupt enable 	
	
		DMA1_Channel4->CCR|=DMA_CCR4_EN;  							/* Enable DMA TX Channel */	
		DMA1_Channel5->CCR|=DMA_CCR5_EN;  							/* Enable DMA RX Channel */	
		rx7kt_transaction=1;

}
/*
*/
void ReadEEPROM_CmdDMA(uint8_t *buffer,uint16_t AdrStart){
		
		cmd7kt_struct.DevAdr=0;
		cmd7kt_struct.Cmd=1;
		cmd7kt_struct.AdrL=(uint8_t)AdrStart;
		cmd7kt_struct.AdrH=AdrStart>>8;
		cmd7kt_struct.Length=0;						// 256 bytes
		
		check_crc(&cmd7kt_struct.DevAdr,5);
	
		DMA1->IFCR|=DMA_IFCR_CTCIF4;//|DMA_IFCR_CTCIF5;		/*Clear TC4 TC5 flag */
		
		DMA1_Channel4->CCR&=~DMA_CCR4_EN;  							/* Disable DMA TX Channel */	
		DMA1_Channel5->CCR&=~DMA_CCR5_EN;  							/* Disable DMA RX Channel */	
		
		DMA1_Channel4->CNDTR=sizeof(cmd7kt_struct);
		
		DMA1_Channel5->CMAR=(uint32_t)buffer;
		DMA1_Channel5->CNDTR=258;
	
		DMA1_Channel5->CCR|=DMA_CCR5_TCIE;							// transfer complete interrupt enable 	
		DMA1_Channel4->CCR|=DMA_CCR4_EN;  							/* Enable DMA TX Channel */	
		DMA1_Channel5->CCR|=DMA_CCR5_EN;  							/* Enable DMA RX Channel */	
		rx7kt_transaction=1;
}
/*
*/

void ReadRAM_CmdDMA(uint8_t *buffer,uint8_t vers_arch){

	cmd7kt_struct.DevAdr=0;
	cmd7kt_struct.Cmd=2;
	cmd7kt_struct.AdrL=0;
	cmd7kt_struct.AdrH=0;
	cmd7kt_struct.Length=0;
	
	check_crc(&cmd7kt_struct.DevAdr,5);
	
	
	DMA1->IFCR|=DMA_IFCR_CTCIF4;//|DMA_IFCR_CTCIF5;		/*Clear TC4 TC5 flag */
	
	DMA1_Channel4->CCR&=~DMA_CCR4_EN;  							/* Disable DMA TX Channel */	
	DMA1_Channel5->CCR&=~DMA_CCR5_EN;  							/* Disable DMA RX Channel */	
	
	DMA1_Channel4->CNDTR=sizeof(cmd7kt_struct);
			
	DMA1_Channel5->CMAR=(uint32_t)buffer;
	DMA1_Channel5->CNDTR=vers_arch+2;
	
	DMA1_Channel5->CCR|=DMA_CCR5_TCIE;							// transfer complete interrupt enable 	
	DMA1_Channel4->CCR|=DMA_CCR4_EN;  							/* Enable DMA TX Channel */	
	DMA1_Channel5->CCR|=DMA_CCR5_EN;  							/* Enable DMA RX Channel */	
	rx7kt_transaction=1;
	

}

/*
*/
static void Send_Cmd(uint8_t* cmd7kt){
		uint8_t i;
		
		while((USART1->SR&USART_SR_TXE)!=USART_SR_TXE) {}
		USART1->DR=0xA5;
		
		for(i=0;i<7;i++)
		{
			while((USART1->SR&USART_SR_TXE)!=USART_SR_TXE) {}
			USART1->DR=*(cmd7kt+i);
		}
		while((USART1->SR&USART_SR_TC)!=USART_SR_TC) {}
		i=USART1->DR;
}
/*
*/
void Сhange_speed_cmd(uint8_t speed){
				
		cmd7kt_struct.DevAdr=0;
		cmd7kt_struct.Cmd=6;
		cmd7kt_struct.AdrL=speed;
		cmd7kt_struct.AdrH=speed;
		cmd7kt_struct.Length=0;
		
		check_crc(&cmd7kt_struct.DevAdr,5);
		Send_Cmd(&cmd7kt_struct.DevAdr);
}

/*
*/
uint8_t Init_connect_7KT(void){
	//uint8_t n=3;
	uint8_t uartdr;
	/* Установим скорость порта 2400 */
	/*	2400=48000000/(16*USARTDIV) USARTDIV=1250  0x4E20   	*/
	/*	2400=8000000/(16*USARTDIV) USARTDIV=3333  0xD05   	*/
	
	cmd7kt_struct.Start=0xA5;
	//while(n--)
		//{
			USART1->CR1&=~(USART_CR1_UE|USART_CR1_TE|USART_CR1_RE);
			USART1->BRR=0xD05;
			USART1->CR1|=USART_CR1_UE|USART_CR1_TE|USART_CR1_RE;
			/*Clear TC flag*/
			USART1->SR&=~USART_SR_TC;
			/*with  followed by a write to the USART_DR register*/
			SendActiv_level();
			while(tx7kt_transaction)		
				__WFI();
			DMA1_Channel4->CCR &=~DMA_CCR4_EN;  							/* Disable DMA TX Channel */	
			DMA1_Channel4->CCR &=~DMA_CCR4_TCIE;							// transfer complete interrupt enable  
			//Delay(2000);
		/* Выдаем команду установки скорости (19200) на низкой 2400 */
			Change_speed_DMA(&uartdr,S9600);
			timeout_delay=1;
			while(rx7kt_transaction)		
				__WFI();
			if(uartdr==6)
			{
				/* Меняем настройки USART STM32  на 19200 */
				/*	19200=8000000/(16*USARTDIV) USARTDIV=416  0x1A0	*/		
			USART1->CR1&=~(USART_CR1_UE|USART_CR1_TE|USART_CR1_RE);
			USART1->BRR=0x341;
			USART1->CR1|=USART_CR1_UE|USART_CR1_TE|USART_CR1_RE;
			/* Повторяем команду установки скорости (9600) уже на новой скорости 9600*/
			Change_speed_DMA(&uartdr,S9600);
			timeout_delay=1;
			while(rx7kt_transaction)
				__WFI();
			if(uartdr==6)
				return 0;			// выход из функции с успешным результатом
			}
	// иначе меняем скорость UART на 9600 и на этой скорости посылаем команду смены на 9600
	/*	9600=8000000/(16*USARTDIV) USARTDIV=833  0x341   	*/
			USART1->CR1&=~(USART_CR1_UE|USART_CR1_TE|USART_CR1_RE);
			USART1->BRR=0x341;
			USART1->CR1|=USART_CR1_UE|USART_CR1_TE|USART_CR1_RE;
			/*Clear TC flag*/
			USART1->SR&=~USART_SR_TC;				
	/* Повторяем команду установки скорости (9600) уже на новой скорости 9600*/
			Change_speed_DMA(&uartdr,S9600);
			timeout_delay=1;
			while(rx7kt_transaction)
				__WFI();
			if(uartdr==6)
			{
				/* Меняем настройки USART STM32  на 19200 */
				/*	19200=8000000/(16*USARTDIV) USARTDIV=416  0x1A0	*/		
				/*USART1->CR1&=~(USART_CR1_UE|USART_CR1_TE|USART_CR1_RE);
				USART1->BRR=0x1A1;
				USART1->CR1|=USART_CR1_UE|USART_CR1_TE|USART_CR1_RE;*/
				return 0;			// выход из функции с успешным результатом
			}
		//}
		return 1;		
}
/*
*/
uint8_t Read_EEPROM(uint8_t * buffer,uint16_t AdrStart,uint16_t AdrStop){
		uint16_t i;
		uint8_t * pcmd7kt=(uint8_t*)&cmd7kt_struct;
	
		cmd7kt_struct.DevAdr=0;
		cmd7kt_struct.Cmd=1;
		cmd7kt_struct.AdrL=(uint8_t)AdrStart;
		cmd7kt_struct.AdrH=AdrStart>>8;
		cmd7kt_struct.Length=0;
			
		while(AdrStart!=AdrStop)
		{
			check_crc(pcmd7kt,5);
			Send_Cmd(pcmd7kt);
			for(i=0;i<(256+2);i++)
			{
				SysTick->LOAD=(2500000*2);
				SysTick->VAL=0;
				while((!(USART1->SR&USART_SR_RXNE))&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG))){}
				if(USART1->SR&USART_SR_RXNE)
					*(buffer+i)=USART1->DR;
				else
					return 1;	
			}
			if(check_crc(buffer,256)==*(uint16_t*)(buffer+256))
			{
				AdrStart+=256;
				cmd7kt_struct.AdrL=(uint8_t)AdrStart;
				cmd7kt_struct.AdrH=AdrStart>>8;
				buffer+=256;
			}
			else
				return 2;
		}
		return 0;
}
/*
*/
uint8_t Read_currentData(uint8_t *buffer){
	uint16_t i;
	uint8_t * pcmd7kt=(uint8_t*)&cmd7kt_struct;
	
	cmd7kt_struct.DevAdr=0;
	cmd7kt_struct.Cmd=2;
	cmd7kt_struct.AdrL=0;
	cmd7kt_struct.AdrH=0;
	cmd7kt_struct.Length=0;
	
	check_crc(pcmd7kt,5);
	Send_Cmd(pcmd7kt);
	for(i=0;i<(64+2);i++)
		{
			SysTick->LOAD=(2500000*2);
			SysTick->VAL=0;
			while((!(USART1->SR&USART_SR_RXNE))&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG))){}
			if(USART1->SR&USART_SR_RXNE)
				*(buffer+i)=USART1->DR;
			else
				return 1;	
		}
	if(check_crc(buffer,64)==*(uint16_t*)(buffer+64))
		return 0;
	else
		return 1;
}
/*
*/
void hex2dec_byte(uint8_t* src,char *dst){
	*(dst+0)=(*src/10)|0x30;
	*(dst+1)=(*src%10)|0x30;
}
/*
*/

void hex2dec_word(uint16_t* src,char *dst){
	*(dst+0)=(*src/10000)|0x30;
	*(dst+1)=((*src%10000)/1000)|0x30;
	*(dst+2)=((*src%1000)/100)|0x30;
	*(dst+3)=((*src%100)/10)|0x30;
	*(dst+4)=(*src%10)|0x30;
}

/*
*/
