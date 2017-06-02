#include "stm32f10x.h"
#include "ffconf.h"
#include "diskio.h"
#include "spi_sdcard.h"


/* Private define ------------------------------------------------------------*/




/* Private typedef -----------------------------------------------------------*/


/* Private variables ---------------------------------------------------------*/
uint8_t CardType;			/* Card type flags */
volatile DSTATUS Stat = STA_NODISK|STA_NOINIT;	/* Disk status */

CSDTypeDef csd;
/* Private function prototypes -----------------------------------------------*/


/* Private functions ---------------------------------------------------------*/
/**********************************************************************************************
*																																															*	
***********************************************************************************************/
static void interface_speed(SPEED speed){

	if(speed==SLOW)
		SPI_SD->CR1|=SPI_CR1_BR_1|SPI_CR1_BR_2;				//SPI_BaudRatePrescaler_256  110   48000000/128=187500*2
	else 
		SPI_SD->CR1&=~SPI_CR1_BR;											//SPI_BaudRatePrescaler_2    000		48000000/2=24000000

}

#if 0
/**********************************************************************************************
*												Card_power																														*	
***********************************************************************************************/
static void card_power(uint8_t on_off){
	
	GPIO_InitTypeDef GPIO_InitStruct;
	if(on_off)
	{
		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_OD;
		GPIO_InitStruct.GPIO_Pin=CardPower_Pin;
		GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;
		GPIO_Init(CardPower_Port,&GPIO_InitStruct);
		// Включаем транзистор и подаем питание на SD карту
		CardPower_Port->BSRR=CardPower_On;
		
	}
	else
	{
		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
		GPIO_InitStruct.GPIO_Pin=CardPower_Pin;
		GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;
		GPIO_Init(CardPower_Port,&GPIO_InitStruct);
		/* Транзистор сам выключается когда переводим вывод в IN_FLOATING */
	
	}
}
#endif
/**********************************************************************************************
*												Power_off																														*	
***********************************************************************************************/
static void power_off(void){

	GPIO_InitTypeDef GPIO_InitStruct;
	
	SPI_I2S_DeInit(SPI_SD);
	SPI_Cmd(SPI_SD,DISABLE);
	RCC_APBPeriphClockCmd_SPI_SD(RCC_APBPeriph_SPI_SD, DISABLE);
	
	/* All SPI-Pins to input with weak internal pull-downs */
	GPIO_InitStruct.GPIO_Pin   = SPI_SD_SCK | SPI_SD_MISO | SPI_SD_MOSI;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPD;
	GPIO_Init(SPI_SD_Port, &GPIO_InitStruct);
	
	//card_power(0);
	
	Stat |= STA_NOINIT;		/* Set STA_NOINIT */
	
}

/**********************************************************************************************
*												Power_on																															*	
***********************************************************************************************/
static void power_on(void){

	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	
	/* Enable SPI clock, SPI1: APB2, SPI2: APB1 */
	RCC_APBPeriphClockCmd_SPI_SD(RCC_APBPeriph_SPI_SD, ENABLE);
	
	//card_power(1);
	
	SysTick->LOAD=250000*6;
	SysTick->VAL=0;
	while(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG)) {}		/* Wait for 250ms */
	
	/* Configure I/O for Flash Chip select */
	GPIO_InitStructure.GPIO_Pin   = SPI_SD_CS;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI_SD_CS_PORT, &GPIO_InitStructure);
	
	/* De-select the Card: Chip Select high */
	DESELECT();

	/* Configure SPI pins: SCK and MOSI with default alternate function (not re-mapped) push-pull */
	GPIO_InitStructure.GPIO_Pin   = SPI_SD_SCK | SPI_SD_MOSI;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
	GPIO_Init(SPI_SD_Port, &GPIO_InitStructure);
	/* Configure MISO as Input with internal pull-up */
	GPIO_InitStructure.GPIO_Pin   = SPI_SD_MISO;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
	GPIO_Init(SPI_SD_Port, &GPIO_InitStructure);

	/* SPI configuration */
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128; // 48000kHz/128=187500*2Hz < 400kHz
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(SPI_SD, &SPI_InitStructure);
	SPI_CalculateCRC(SPI_SD, DISABLE);
	SPI_Cmd(SPI_SD, ENABLE);
	
}
/**********************************************************************************************
*			Transmit/Receive a byte to MMC via SPI  																								*																					*	
***********************************************************************************************/
static uint8_t spi_rw(uint8_t data){

	SPI_SD->DR=data;
	while((SPI_SD->SR&SPI_SR_RXNE)!=SPI_SR_RXNE) {}
	return SPI_SD->DR;
}
/*
*/
static uint8_t xmit_spi(uint8_t data){
		SPI_SD->DR=data;
		while((SPI_SD->SR&SPI_SR_RXNE)!=SPI_SR_RXNE) {}
		return SPI_SD->DR;	
}
/*
*/
static uint8_t rcvr_spi(void){

	SPI_SD->DR=0xff;
	while((SPI_SD->SR&SPI_SR_RXNE)!=SPI_SR_RXNE) {}
	return SPI_SD->DR;

}
/*
*/
static uint8_t wait_ready(void){

	uint8_t res;
	SysTick->LOAD=500000*6;     //500ms 
	SysTick->VAL=0;
	rcvr_spi();							// Byte space при записи в любом случае
	do
		res=rcvr_spi();
	while((res!=0xff)&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG)));
	return res;

}

/************************************************************************************************
				Transmit/Receive Block using DMA (Platform dependent. STM32 here)     									
*************************************************************************************************/
static void dma_transfer(DIRECTION dir,const uint8_t* buff,uint32_t btr){

		DMA_InitTypeDef DMA_InitStruct;
		
		uint16_t workbyte[]={0xffff};
	
	DMA_InitStruct.DMA_BufferSize=btr;
	DMA_InitStruct.DMA_M2M=DMA_M2M_Disable;
	DMA_InitStruct.DMA_Mode=DMA_Mode_Normal;
	DMA_InitStruct.DMA_Priority=DMA_Priority_VeryHigh;
	DMA_InitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_PeripheralBaseAddr=(uint32_t)&SPI_SD->DR;
	DMA_InitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;
	
	DMA1->IFCR|=DMA_IFCR_CTCIF2|DMA_IFCR_CTCIF3; /*Clear TC2 TC3 flag */
		
	if(dir) // receive		spi_rx DMA1_Channel2   spi_tx DMA1_Channel3
	{	
		DMA_InitStruct.DMA_MemoryBaseAddr=(uint32_t)buff;
		DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;
		DMA_InitStruct.DMA_DIR=DMA_DIR_PeripheralSRC;
		DMA_Init(DMA1_Channel2,&DMA_InitStruct);
				
		DMA_InitStruct.DMA_MemoryBaseAddr=(uint32_t)&workbyte;
		DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Disable;
		DMA_InitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
		DMA_Init(DMA1_Channel3,&DMA_InitStruct);
	}
	else		//transmit  
	{
		DMA_InitStruct.DMA_MemoryBaseAddr=(uint32_t)&workbyte;
		DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Disable;
		DMA_InitStruct.DMA_DIR=DMA_DIR_PeripheralSRC;
		DMA_Init(DMA1_Channel2,&DMA_InitStruct);
				
		DMA_InitStruct.DMA_MemoryBaseAddr=(uint32_t)buff;
		DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;
		DMA_InitStruct.DMA_DIR=DMA_DIR_PeripheralDST;
		DMA_Init(DMA1_Channel3,&DMA_InitStruct);
	}
	DMA1_Channel2->CCR|=DMA_CCR2_EN;  /* Enable DMA RX Channel */
	DMA1_Channel3->CCR|=DMA_CCR3_EN;	/* Enable DMA TX Channel */
	/* Enable SPI TX/RX request */
	SPI_SD->CR2|=SPI_CR2_RXDMAEN|SPI_CR2_TXDMAEN;
	
	/* Wait until DMA1_Channel 2 Transfer Complete */
	/* Wait until DMA1_Channel 3 Transfer Complete */
	while((DMA1->ISR&DMA_ISR_TCIF2)!=DMA_ISR_TCIF2) {}
	
	DMA1_Channel2->CCR&=~DMA_CCR2_EN;  /* Disable DMA RX Channel */
	DMA1_Channel3->CCR&=~DMA_CCR3_EN;	/* Disable DMA TX Channel */
	
	/* Disable SPI TX/RX request */
	SPI_SD->CR2&=~(SPI_CR2_RXDMAEN|SPI_CR2_TXDMAEN);
}

/************************************************************************************************
								Receive a data packet from MMC                           												
*************************************************************************************************/
static uint8_t rcvr_datablock(uint8_t *buff,uint32_t btr){

	uint8_t token;
	
	SysTick->LOAD=100000*6;							//100ms
	SysTick->VAL=0;
	do{
	
		token=rcvr_spi();									
		} while((token==0xff)&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG)));
																		// Ждем пока DO станет 0xFF или таймер закончится 100 ms 
	 if(token!=0xfe) return 0;				// Если приняли 0xfe значит это data tocken если нет выход	
																		// с ошибкой
		dma_transfer(RX,buff,btr);			// Принимаем блок из btr байт
		rcvr_spi();											/* Discard CRC */
		rcvr_spi();	
	return 1;
}

/************************************************************************************************
								Send a data packet to MMC                                       								
*************************************************************************************************/
static uint8_t xmit_datablock(const uint8_t *buff,uint8_t token){
	
	uint8_t resp;
	
	if(wait_ready()!=0xff) 	// Ждем готовности линии DO=0xff или таймаут 500	Byte space уже отправлен  				
		return 0;							// Если время вышло и готовности карты нет DO!=0xff выход с ошибкой
	
	xmit_spi(token);			// Шлем data token 0xfc 0xfe для CMD24 CMD25  	
	if(token!=0xfd)				// Если stop tran token 0xfd то просто выходим
	{
		dma_transfer(TX,buff,512);
		spi_rw(0xFF);					/* CRC (Dummy) */
		spi_rw(0xFF);
		resp = rcvr_spi();				/* Receive data response */
		if ((resp & 0x1F) != 0x05)		/* If not accepted, return with error */
			return 0;
	}
	return 1;	
}

/**********************************************************************************************
									Send a command packet to MMC                                          
***********************************************************************************************/
uint8_t send_cmd(uint8_t cmd,uint32_t arg){

	uint8_t n,res;
	
	if (cmd & 0x80) {	/* ACMD<n> is the command sequence of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	DESELECT();
	SELECT();
	if (wait_ready() != 0xFF) {
		return 0xFF;
	}
	/* Send command packet */
	xmit_spi(cmd);						/* Start + Command index */
	xmit_spi((uint8_t)(arg >> 24));		/* Argument[31..24] */
	xmit_spi((uint8_t)(arg >> 16));		/* Argument[23..16] */
	xmit_spi((uint8_t)(arg >> 8));			/* Argument[15..8] */
	xmit_spi((uint8_t)arg);				/* Argument[7..0] */
	n = 0x01;										/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xmit_spi(n);
	
	/* Receive command response */
	if (cmd == CMD12) 
		rcvr_spi();		/* Skip a stuff byte when stop reading */
	n = 8;								/* Wait for a valid response in timeout of 10 attempts */
	do
		res = rcvr_spi();		// Шлем 0xff пока DO карты в high состоянии
	while ((res & 0x80) && --n);	// Если старший бит 0 значит карта выдала ответ res	

	return res;			/* Return with the response value */
}

/***************************************************************************************************
											SD_Card initialize						
****************************************************************************************************/
DSTATUS sd_card_init(void){
	uint8_t n,ty,cmd;	
	uint8_t ocr[4];
		//if(drv) return STA_NOINIT;
		//if (Stat & STA_NODISK) return Stat;	/* No card in the socket */

		power_on();
		interface_speed(SLOW);
		for(n=0;n<10;n++)
			rcvr_spi();							// 80 Dummy clocks;
		ty=0;
		if(send_cmd(CMD0,0)==0x1){
			if(send_cmd(CMD8,0x1AA)==0x1)
			{	//Ver2.00 or later SD 
				for(n=0;n<4;n++)
					ocr[n]=rcvr_spi();	// Читаем 4 иладшие байта в ответе R7 на CMD8 		
				if(ocr[2]==0x1&&ocr[3]==0xAA)
				{	// Card with compatible vltage range VDD 2.7-3.6
					// Шлем ACMD41 с битом HCS=1 пока карта не выйдет из Idle state или таймер не закончится
					SysTick->LOAD=1000000*6;							//1000ms
					SysTick->VAL=0;
					do{
						n=send_cmd(ACMD41,1UL<<30);
						}	while(n&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG)));
					if(send_cmd(CMD58,0)==0){  /* Check CCS bit in the OCR */
						for(n=0;n<4;n++)
							ocr[n]=rcvr_spi();
						ty=(ocr[0]&0x40)?CT_SD2 | CT_BLOCK : CT_SD2;
					}
				}
			}
			else
			{ //CMD8 Illegal command  -> Ver1.x SD or not SD 
				if(send_cmd(ACMD41,0)<=1){	//Ver1.x SD	
					ty=CT_SD1; 
					cmd=ACMD41;
					}					
				else{											// MMC Ver.3
				ty=CT_MMC;
				cmd=CMD1;	
				}
				SysTick->LOAD=1000000*6;							//1000ms
				SysTick->VAL=0;
				do{
						n=send_cmd(cmd,0);
					}	while(n&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG)));
				if(!n||send_cmd(CMD16, 512)!=0)
					ty=0;
			}				
		}
		CardType=ty;	
		DESELECT();
		if(ty)
		{									/* Initialization succeeded */
			Stat &= ~STA_NOINIT;		/* Clear STA_NOINIT */
			interface_speed(FAST);
		}
		else				/* Initialization failed */
		{
			power_off();
		
		}
		return Stat;
}
/****************************************************************************************************
									SD_Card Read Sector/block
*****************************************************************************************************/
DRESULT SPI_SD_ReadBlock(uint32_t address,uint8_t *buff/*,uint16_t blocksize*/){
	DRESULT res=RES_ERROR;
	
	//if(!(CardType&CT_BLOCK)) address*=512;
	if(send_cmd(CMD17,address)==0){	 /* READ_SINGLE_BLOCK */
		if(rcvr_datablock(buff,512)){
			res=RES_OK;
		}
	}
	DESELECT();
	return res; 
}

/****************************************************************************************************
									SD_Card Read Multi Sector/block
*****************************************************************************************************/
DRESULT SPI_SD_ReadMultiBlock(uint32_t address,uint8_t *buff,/*uint16_t blocksize,*/uint32_t NumberOfBlocks){
	DRESULT res=RES_ERROR;
	
	//if(!(CardType&CT_BLOCK)) address*=512;
	if(send_cmd(CMD18,address)==0){	/*READ_MULTI_BLOCKS*/
		do{
				if (!rcvr_datablock(buff, 512)) {
					break;
				}
				buff += 512;
			} while (--NumberOfBlocks);
		send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		res=RES_OK;
	}
	DESELECT();
	if(NumberOfBlocks) return res;
		
	return res;
}
/****************************************************************************************************
									SD_Card Write Sector/block
*****************************************************************************************************/
DRESULT SPI_SD_WriteBlock (uint32_t address,const uint8_t *buff/*,uint16_t blocksize*/){
	DRESULT res=RES_ERROR;
	
	//if(!(CardType&CT_BLOCK)) address*=512;
	if(send_cmd(CMD24,address)==0){	 /* WRITE_SINGLE_BLOCK */
		if(xmit_datablock(buff,0xFE)==1){
			res=RES_OK;
		}
	}
	DESELECT();
	return res; 
}

/****************************************************************************************************
									SD_Card Read Multi Sector/block
*****************************************************************************************************/
DRESULT SPI_SD_WriteMultiBlock(uint32_t address,const uint8_t *buff,/*uint16_t blocksize,*/uint32_t NumberOfBlocks){
	DRESULT res=RES_ERROR;
	
	//if(!(CardType&CT_BLOCK)) address*=512;
	if(CardType&CT_SDC)			
		send_cmd(ACMD23,NumberOfBlocks);
	
	if(send_cmd(CMD25,address)==0){	 /* WRITE_MULTI_BLOCK */
		do{
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--NumberOfBlocks);
		if (xmit_datablock(0, 0xFD)&&(!NumberOfBlocks))	/* STOP_TRAN token */
				res=RES_OK;
		}
	DESELECT();
	return res;	
}

/*

*/
void SPI_SD_GETCSD(void){
	uint8_t tcsd[16];
	uint16_t mult;
	
if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(tcsd, 16)){
	if(tcsd[0]>>6==0){	// CSD struct vers 1.0 
		csd.csd_type=1;
		csd.sector_len=1<<(tcsd[5]&0xf);
		
		mult=((tcsd[9]&0x03)<<1)|((tcsd[10]&0x80)>>7);
		mult+=2;
		mult=1<<mult;
		csd.sector_count=((((uint16_t)(tcsd[6]&0x03)<<10)|((uint16_t)tcsd[7]<<2)|(tcsd[8]>>6))+1)*mult;
		csd.erase_size=((((tcsd[10]<<1)&0x7f)|((tcsd[11]&0x80)>>7))+1)*csd.sector_len;		// KB
		csd.erase_block_en=(tcsd[10]>>6)&0x1;
		csd.card_capacity=csd.sector_count*csd.sector_len;
		csd.read_current_min=(tcsd[8]>>3)&0x7;
		csd.read_current_max=tcsd[8]&0x07;
		csd.write_current_min=(tcsd[9]>>5)&0x7;
		csd.write_current_max=(tcsd[9]>>2)&0x7;
	
	
	}
	else{ 	// CSD struct vers 2.0
		csd.csd_type=2;
		csd.sector_len=512;
		csd.sector_count = (uint32_t)(tcsd[9]|(tcsd[8] << 8)|(tcsd[7]<<16)+1)<<10;
		csd.card_capacity=csd.sector_count*512;
		}
	}
}
/*

*/
void sdcard_reinitial(void){
	uint8_t i;
	
	//SPI_SD->CR1|=SPI_CR1_BR_1|SPI_CR1_BR_2;	//SPI_BaudRatePrescaler_128  110   24000000/128=187500
	if(send_cmd(CMD0,0)==1)
		{	
			interface_speed(SLOW);
			// Шлем ACMD41 с битом HCS=1 пока карта не выйдет из Idle state или таймер не закончится
			SysTick->LOAD=1000000*6;							//1000ms
			SysTick->VAL=0;
			do{
					i=send_cmd(ACMD41,1UL<<30);
				}	while(i&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG)));
			DESELECT();
			interface_speed(FAST);			//SPI_BaudRatePrescaler_2    000		24000000/2=12000000//Програмный сброс после операций чтения записи для снижения потребления 
		}
}
/*
*/
