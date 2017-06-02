#include <stm32f10x.h>
#include "stm32f10x_conf.h"

#include "main.h"
#include "usb_pwr.h"
#include "usb_lib.h"
#include "LCD_Driver.h"
#include "RS232.h"

#include "ff.h"
DEV7KT_TypeDef dev7kt;

uint8_t lastread_status=0x17;
uint8_t volatile Sleep;
uint8_t volatile Wkup_Ext;
uint8_t volatile refresh;
_7KTS_STATE volatile _7kts_State;
_MENU_PUNKT volatile menu_punkt;

volatile uint8_t battview_mode;

uint16_t global_crc;

extern volatile uint8_t time_backlight;
volatile uint8_t result;
volatile uint8_t timeout_delay;
volatile uint16_t count_AddrEEPROM; 
extern volatile uint8_t count_DMAtransfer;
extern volatile uint8_t rx7kt_transaction;
CMD7KT_TypeDef  cmd7kt_struct;
uint8_t current_buff[8192+2];

char filename[]="0:     /     _20  -  -  _  -  .bin";

FATFS fs;
FRESULT fresult=FR_OK;
FIL fil;
DIR	dir;
FILINFO finfo;

extern uint8_t send_cmd(uint8_t cmd,uint32_t arg);
extern CSDTypeDef csd;
uint16_t SD_SECTOR_SIZE=512;

uint16_t ADC1Buff[4];
uint8_t VBAT[5];
extern volatile uint8_t batt_indic;
void assert_failed(uint8_t* file, uint32_t line)
{
	while(1){};
}

void Init_Periph(void);
void ADC1_Init (void);
void RTC_Init(void);
void USART1_Init(void);
void Wellcom(void);
FRESULT SD_Card_Selftest(void);
void DateCalc(void);
void Change_Freq(_7KTS_SPEED speed);


 int main (void){
	uint8_t i;
	FRESULT res;
	uint32_t bw;
	char SN[6]="     ";
	
	// Настроим SysTick сбросим флаг CLKSOURCE выберем источник тактирования AHB/8
	SysTick->CTRL &=~SysTick_CTRL_CLKSOURCE;
	SysTick->CTRL |=SysTick_CTRL_ENABLE_Msk;
	Init_Periph();
	ADC1_Init();
	USART1_Init();
	SCB->CCR|=SCB_CCR_USERSETMPEND_Msk;
	DBGMCU->CR|=DBGMCU_CR_DBG_SLEEP;
	DBGMCU->CR|=DBGMCU_CR_DBG_STOP;
	
		
	VBAT[1]=0x2E;
	/* Инициализация дисплея MT-16S2D*/
	LCD_Init();
	/*	Приветствие на экране*/
	//Wellcom();
	if(GPIO_ReadInputDataBit(CardDetect_Port, CardDetect_Pin)==RESET)
		Stat=STA_NOINIT;
	else
	{
		LCD_PutTextAT(text_NoSDCard,0x1);
		Stat=STA_NODISK;
		_7kts_State=NODISK_MODE;
		Delay(2500000);
		LCD_Clear();	
	}
	if(Stat&STA_NOINIT)
	{
		if(disk_initialize(0)) 
		{
			LCD_PutTextAT(text_SDCard_Error,0x0);
			_7kts_State=NODISK_MODE;
			Delay(1000000);
			LCD_Clear();	
		}
			else
			SD_Card_Selftest();
	}
	EXTI_ClearITPendingBit(EXTI_Line1|EXTI_Line6|EXTI_Line10|EXTI_Line11);
	
	NVIC_ClearPendingIRQ(EXTI1_IRQn);
	NVIC_SetPriority(EXTI1_IRQn,3);
	NVIC_EnableIRQ(EXTI1_IRQn);
	
	NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
	NVIC_SetPriority(EXTI15_10_IRQn,3);
	NVIC_EnableIRQ(EXTI15_10_IRQn);
	
	NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
	NVIC_SetPriority(EXTI9_5_IRQn,3);
	NVIC_EnableIRQ(EXTI9_5_IRQn);
	
	NVIC_SetPriority(DMA1_Channel4_IRQn,2);
	NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	
	NVIC_SetPriority(DMA1_Channel5_IRQn,1);
	NVIC_EnableIRQ(DMA1_Channel5_IRQn);
	
	USB_Interrupts_Config();
	if(USB_DETECT_PORT->IDR & USB_DETECT_PIN)
	{
		EXTI->IMR&=~(EXTI_IMR_MR10|EXTI_IMR_MR6);
		USB_Init();
		NVIC->STIR=7;
	}
	else	
	_7kts_State=RS232_MODE;
	
while(1){
	
	 if(_7kts_State==USB_MODE)
		{
			LCD_Clear();
			LCD_PutTextAT("USB",0x1);
			LCD_PutTextAT(text_Port,0x41);
			while(refresh==0)
			{	
				LCD_PutCharAT((char)batt_indic,7);
				__WFI();
			}
			refresh=0;	
		}
	else if(_7kts_State ==BATLOW_MODE)
		{
			LCD_Clear();
			LCD_PutTextAT(text_Low,0x0);
			LCD_PutTextAT(text_CHARGE_mode,0x40);
			while(refresh==0)
			{
				__WFI();
			}
			refresh=0;
		}	
	else if(_7kts_State ==RS232_MODE)	// ATTACHED=1 SUSPENDED=3 CONFIGURED=5
		{
			
			EXTI->IMR|=EXTI_IMR_MR10|EXTI_IMR_MR6;
			LCD_Clear();
			LCD_PutCharAT((char)batt_indic,7);
			LCD_PutTextAT("COM",0x1);
			LCD_PutTextAT(text_Port,0x41);
			LCD_PutCharAT(lastread_status,0x47);
			switch(menu_punkt)
			{
				case READ_MENU:
					LCD_Clear();
					LCD_PutTextAT(text_Read_7KT,0x0);	// Читать
					LCD_PutTextAT("7KT ?",0x41);			//  7КТ ? 
					timeout_delay=3;
					while((timeout_delay)&&(menu_punkt==READ_MENU))
						__WFI();
					if(menu_punkt!=READ_MENU)				
						break;
					EXTI->IMR&=~EXTI_IMR_MR10;
										
					LCD_Clear();
					LCD_PutTextAT(text_Connect,0x0);	// Связь... 
					
					lastread_status=0x78;
					if(Init_connect_7KT())
					{
						LCD_Clear();
						LCD_PutTextAT(text_Error,0x1);				// Ошибка
						LCD_PutTextAT(text_Connecting,0x41); 	// связи!
						time_backlight=10;
						timeout_delay=3;
						while(timeout_delay)	
							__WFI();											//	в sleep пока таймер считает 2-3 сек.
						menu_punkt=NODATA_MENU;
						refresh=1;		
						break;
					}
					
					//DMA_Usart_Init();	
/******************************************** Read EEPROM 1st page *************************************************************************/
					global_crc=1;
					timeout_delay=2;
					ReadEEPROM_CmdDMA(current_buff,0);								// Настраиваем DMA для отправки команды чтения eeprom 256 bytes
					while(rx7kt_transaction)		
						__WFI();
					if(check_crc(current_buff,256)==*(uint16_t*)(current_buff+256))
					{
						dev7kt.LengthArch=*(current_buff+0);
						if(*(current_buff+1)==2)
						{
							dev7kt.VersArch=90;																						// 90 байт+2 CRC на выдачу nтекущих		если W8
							hex2dec_word((uint16_t*)(current_buff+2),(filename+2));   		// добавляем серийный номер прибора в строку для имени файла
							hex2dec_word((uint16_t*)(current_buff+2),(filename+8));   		// добавляем серийный номер прибора в строку для имени файла
							hex2dec_word((uint16_t*)(current_buff+2),SN);									// добавляем серийный номер прибора в строку для имени файла
							
							hex2dec_byte((current_buff+64),(filename+16));						// добавляем год в строку для имени файла
							dev7kt.Year=*(current_buff+64);
							hex2dec_byte((current_buff+65),(filename+19));						// добавляем месяц в строку для имени файла
							dev7kt.Month=*(current_buff+65);
							hex2dec_byte((current_buff+66),(filename+22));						// добавляем день в строку для имени файла
							dev7kt.Day=*(current_buff+66);
							hex2dec_byte((current_buff+67),(filename+25));						// добавляем час в строку для имени файла
							dev7kt.Hour=*(current_buff+67);
						}
						else
						{
							dev7kt.VersArch=64;				// 64 байт+2 CRC на выдачу nтекущих		если W4
							hex2dec_word((uint16_t*)(current_buff+2),(filename+2));   		// добавляем серийный номер прибора в строку для имени файла
							hex2dec_word((uint16_t*)(current_buff+2),(filename+8));   		// добавляем серийный номер прибора в строку для имени файла
							hex2dec_word((uint16_t*)(current_buff+2),SN);									// добавляем серийный номер прибора в строку для имени файла
							
							hex2dec_byte((current_buff+80),(filename+16));						// добавляем год в строку для имени файла
							dev7kt.Year=*(current_buff+80);
							hex2dec_byte((current_buff+81),(filename+19));						// добавляем месяц в строку для имени файла
							dev7kt.Month=*(current_buff+81);
							hex2dec_byte((current_buff+82),(filename+22));						// добавляем день в строку для имени файла
							dev7kt.Day=*(current_buff+82);
							hex2dec_byte((current_buff+83),(filename+25));						// добавляем час в строку для имени файла
							dev7kt.Hour=*(current_buff+83);
						}
					}
					else
					{
						LCD_Clear();
						LCD_PutTextAT("EEPROM",0x1);
						LCD_PutTextAT("error",0x41);
						time_backlight=10;
						timeout_delay=3;
						while(timeout_delay)	
							__WFI();											//	в sleep пока таймер считает 1 сек.
						menu_punkt=NODATA_MENU;
						refresh=1;		
						break;
					}
/*********************************************************************************************************************************************/				
/*********************************************  Read RAM    ***********************************************************************/
					timeout_delay=2;
					ReadRAM_CmdDMA(current_buff,dev7kt.VersArch);
					while(rx7kt_transaction)						// Ждем когда DMA примет 66 байт 
							__WFI();
					if(check_crc(current_buff,dev7kt.VersArch)==*(uint16_t*)(current_buff+dev7kt.VersArch))
					
					{
						check_global_crc(current_buff,dev7kt.VersArch);
						
						hex2dec_byte((current_buff+dev7kt.VersArch-2),(filename+28));				// добавляем минуты в строку для имени файла
						dev7kt.Minute=*(current_buff+dev7kt.VersArch-2);
						dev7kt.Sec=*(current_buff+dev7kt.VersArch-1);
					}
					else
					{
						LCD_Clear();
						LCD_PutTextAT("RAM",0x2);
						LCD_PutTextAT("error",0x41);
						time_backlight=10;
						timeout_delay=3;
						while(timeout_delay)	
							__WFI();											//	в sleep пока таймер считает 1 сек.
						menu_punkt=NODATA_MENU;
						refresh=1;		
						break;
					}
/*******************************************************************************************************************************************/			
					Change_Freq(PLL);
					if(f_opendir (&dir,SN)!=FR_OK)
					{
						if(f_mkdir(SN)!=FR_OK)
							{
								LCD_Clear();
								LCD_PutTextAT(text_Error,0x1);			// Ошибка
								LCD_PutTextAT(text_DIR,0x0);				//каталога
								time_backlight=10;
							  timeout_delay=3;
								while(timeout_delay)	
									__WFI();											//	в sleep пока таймер считает 1 сек.
								menu_punkt=NODATA_MENU;
								refresh=1;
								break;		
							}
					}
					res=f_open(&fil,filename,FA_CREATE_NEW|FA_READ|FA_WRITE);
					res=f_write(&fil,current_buff,dev7kt.VersArch,&bw);
					if(res!=FR_OK) 
					{
						LCD_Clear();
						LCD_PutTextAT(text_Error,0x1);			// Ошибка			
						LCD_PutTextAT(text_File,0x41);				//  файла 
						time_backlight=10;
						timeout_delay=3;
						while(timeout_delay)	
							__WFI();											//	в sleep пока таймер считает 1 сек.
						menu_punkt=NODATA_MENU;
						refresh=1;		
						break;
					}
					sdcard_reinitial();							//Програмный сброс после операций чтения записи и переинициализация для снижения потребления 
					
					Change_Freq(HSE);
					LCD_Clear();
					LCD_PutTextAT(text_Reading__,0x0);		// Чтение..
					Set_DDRAM_Address (0x40);
					
					result=0;
					count_AddrEEPROM=0;
					for(i=0;i<dev7kt.LengthArch;i++)
					{
						count_DMAtransfer=0;
						while(count_DMAtransfer<32)		
						{
							timeout_delay=2;
							ReadEEPROM_CmdDMA((current_buff+count_DMAtransfer*256),count_AddrEEPROM);								// Настраиваем DMA для отправки команды чтения eeprom 256 bytes
							while(rx7kt_transaction)
								__WFI();
							if(result)
								break;
							if(check_crc((current_buff+count_DMAtransfer*256),256)!=*(uint16_t*)(current_buff+256+count_DMAtransfer*256))		//result=Read_EEPROM(current_buff,i*4096,(4096*(i+1)));
							{
								result=2;
								break;
							}
							count_DMAtransfer++;	
						}
						
						if(result==0)
						{
							
							Change_Freq(PLL);
							if(i==(dev7kt.LengthArch-1))
							{
								check_global_crc(current_buff,8190);
								*(uint16_t*)(current_buff+8190)=global_crc;
							}
							else
								check_global_crc(current_buff,8192);
							
							f_lseek(&fil,(8192*i+dev7kt.VersArch));
							f_write(&fil,current_buff,8192,&bw);
							sdcard_reinitial();
							Change_Freq(HSE);		
							
							if(dev7kt.LengthArch==8)
								LCD_PutChar('>');
							else
							{
								LCD_PutChar('>');
								LCD_PutChar('>');
							}
							time_backlight=17;
						}	
						else if(result==1)
						{
							TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку
							LCD_Clear();
							LCD_PutTextAT(text_Error,0x1);				// Ошибка
							LCD_PutTextAT(text_Connecting,0x41); 	// связи!
							time_backlight=10;
							timeout_delay=3;
							while(timeout_delay)	
								__WFI();											//	в sleep пока таймер считает 1 сек.
							f_close(&fil);
							f_unlink(filename);
							menu_punkt=NODATA_MENU;
							refresh=1;		
							break;
						}
						else if(result==2)
						{
							TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку
							LCD_Clear();
							LCD_PutTextAT(text_Error,0x0);				// Ошибка
							LCD_PutTextAT("CRC !",0x41); 					//  CRC !
							time_backlight=10;
							timeout_delay=3;
							while(timeout_delay)	
								__WFI();											//	в sleep пока таймер считает 1 сек.
							f_close(&fil);
							f_unlink(filename);
							menu_punkt=NODATA_MENU;
							refresh=1;		
							break;
						}
					}
					if(result)
					{
						sdcard_reinitial();						//Програмный сброс после операций чтения записи для снижения потребления 
						break;
					}
					
					LCD_Clear();
					if(SN[0]!=0x30)
						LCD_PutTextAT(SN,0x1);
					else
						LCD_PutTextAT(&SN[1],0x2);
					LCD_PutTextAT(text_Readed_OK,0x41);				// считан !
					f_close(&fil);
					sdcard_reinitial();												//Програмный сброс после операций чтения записи для снижения потребления 
					TIM_PWM->CCER|=TIM_CCER_CC4E;							//Включаем подсветку
					lastread_status=0x17;
					time_backlight=20;
					timeout_delay=5;
					while(timeout_delay)	
						__WFI();											//	в sleep пока таймер считает 1 сек.
					menu_punkt=NODATA_MENU;
					refresh=1;		
				break;	
				case BATVIEW_MENU:
					LCD_Clear();
					LCD_PutTextAT(text_VAKB,0x0);				 // Батарея
					LCD_PutValueAT(VBAT,0x41,5);
					timeout_delay=4;
					while((timeout_delay)&&(menu_punkt==BATVIEW_MENU))
						__WFI();
					if(menu_punkt!=BATVIEW_MENU)				
						break;
					LCD_Clear();
					menu_punkt=NODATA_MENU;
					refresh=1;		
				break;	
			
				default:
				break;	
			}	
			Change_Freq(HSE);
			while(refresh==0)
				{
					__WFI(); 
				}
				refresh=0;
		}
	else if(_7kts_State==NODISK_MODE)
		{
			EXTI->IMR|=EXTI_IMR_MR6;
			EXTI->IMR&=~EXTI_IMR_MR10;
			f_mount (0,0);
			LCD_Clear();
			LCD_PutTextAT(text_NoSDCard,0x1);
			LCD_PutCharAT((char)batt_indic,7);
			while(refresh==0)
				{
				__WFI();
				}
				refresh=0;
		}
	else if(_7kts_State==SDINIT_MODE)
		{
			LCD_Clear();	
			Change_Freq(PLL);
			disk_initialize(0);
			SD_Card_Selftest();
			menu_punkt=NODATA_MENU;
			_7kts_State=RS232_MODE;	
		}
	else if(_7kts_State==CHARGE_MODE)	
		{
			LCD_Clear();
			LCD_PutTextAT(text_CHARGE_mode,0x41);
			
			
			while(refresh==0)
			{
				LCD_PutCharAT((char)batt_indic,7);
				__WFI();
			}
			refresh=0;
		}
	}
}


/****************************************************************/
/*		    				Настройка конфигурации  периферийных модулей	*/
/****************************************************************/
void Init_Periph(void){
	
	GPIO_InitTypeDef GPIO_InitStruct;
	EXTI_InitTypeDef EXTI_InitStruct;
		
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_Div1);
	RCC_AHBPeriphClockCmd(/*RCC_AHBPeriph_CRC|*/
												RCC_AHBPeriph_DMA1,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB|
												 RCC_APB1Periph_TIM_PWM|	
												 RCC_APB1Periph_PWR|		
												 RCC_APB1Periph_BKP|
												 RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO|
												 RCC_APB2Periph_USART1|
												 RCC_APB2Periph_GPIOA|
												 RCC_APB2Periph_GPIOB	,ENABLE);
	/* Переведем неиспользуемые порты в режим AIN	*/
	RCC->APB2ENR|=RCC_APB2ENR_IOPCEN|RCC_APB2ENR_IOPDEN|RCC_APB2ENR_IOPEEN;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AIN;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_All;
	GPIO_Init(GPIOC,&GPIO_InitStruct);
	GPIO_Init(GPIOD,&GPIO_InitStruct);
	GPIO_Init(GPIOE,&GPIO_InitStruct);
		
	/*	Настройка вывода PB11 для кнопки вывода из спящего режима	*/
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_11;
	GPIO_Init(GPIOB,&GPIO_InitStruct);
			
	/*Настройка вывода  SW_POWER_PIN для управления  EN NCP551SN33   */
	GPIO_InitStruct.GPIO_Pin =SW_POWER_PIN;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(SW_POWER_PORT, &GPIO_InitStruct);
	
	GPIO_SetBits(SW_POWER_PORT, SW_POWER_PIN); // включаем  NCP551SN33
	
	/*Сделаем REMAP для PB3 PB4 освободив их от JTDO NJTRST */
	AFIO->MAPR|=AFIO_MAPR_SWJ_CFG_1|AFIO_MAPR_SPI1_REMAP;
	/*Сделаем REMAP для SPI1  на выводы PB3=SCK  PB4=MISO PB5=MOSI */
	
	
	
	/* Вывод CHARGE_INDIC_PIN в input floating*/
	GPIO_InitStruct.GPIO_Pin=CHARGE_INDIC_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(CHARGE_INDIC_PORT,&GPIO_InitStruct);
	
	/* Configure the EXTI line 18 connected internally to the USB IP */
  /*EXTI_ClearITPendingBit(EXTI_Line18);
  EXTI_InitStruct.EXTI_Line = EXTI_Line18;
  EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStruct);*/

/*	Настроим вывод PB6 как CardDetect	*/
		GPIO_InitStruct.GPIO_Pin=CardDetect_Pin;
		GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;			
		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
		GPIO_Init(CardDetect_Port,&GPIO_InitStruct);		
	/*EXTI_Line1 PB1 */
	/*EXTI_Line6 PB6 */
	EXTI_InitStruct.EXTI_Line = EXTI_Line1|EXTI_Line6;
  EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStruct);
		
	/*EXTI_Line10 EXTI_Line11 кнопка на ножке PB10 PB11 */
	EXTI_InitStruct.EXTI_Line =EXTI_Line10|EXTI_Line11;
  EXTI_InitStruct.EXTI_Mode=EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStruct);
	
	AFIO->EXTICR[0]=AFIO_EXTICR1_EXTI1_PB;
	AFIO->EXTICR[1]=AFIO_EXTICR2_EXTI6_PB;
	AFIO->EXTICR[2]=AFIO_EXTICR3_EXTI10_PB|AFIO_EXTICR3_EXTI11_PB;
	
/************************************************************************************************
*        Настройка таймера TIM2 в качестве задающего секундные интервалы эмуляция RTC           *
*************************************************************************************************/
	
	//Зададим коэффициент деления для CK_CNT=CK_PSC/(PSC[15:0]+1)  48000000/(47999+1) =1000Hz
	TIM2->PSC=47999;
	/* С частотой 1000 Гц будет считать до 1000*/
	TIM2->ARR=1000;					// В итоге период обновления получим 2 сек.
	//Источник синхронизации внешнего сигнала CK_INT
	TIM2->SMCR &=~TIM_SMCR_SMS;		//	SMS=000: Slave mode disabled
	
	TIM2->DIER |= TIM_DIER_UIE; //разрешаем прерывание от таймера по Update event
	TIM2->EGR = TIM_EGR_UG;			//генерируем "update event". ARR и PSC грузятся из предварительного в теневой регистр. 
	TIM2->SR&=~TIM_SR_UIF; 			//Сбрасываем флаг UIF
	
	TIM2->CR1 |= TIM_CR1_CEN; 	// Начать отсчёт!	
	NVIC_ClearPendingIRQ(TIM2_IRQn);
	NVIC_SetPriority(TIM2_IRQn,2);
	NVIC_EnableIRQ(TIM2_IRQn);
	
	//
/**********************************************************************************************/
/*					 Вывод TIM_PWM_CH4  PWM управления подсветкой LED																		*/
/**********************************************************************************************/
	GPIO_InitStruct.GPIO_Pin=LCD_PWM;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init(LCD_PORT_LED,&GPIO_InitStruct);
	/*Настроим СР4 таймера TIM_PWM для PWM управления подсветкой LCD*/
	//Зададим коэффициент деления для CK_CNT=CK_PSC/(PSC[15:0]+1)  48000000/(479+1) =100kHz
	TIM_PWM->PSC=479;
	TIM_PWM->ARR=500;
	TIM_PWM->CCR4=250;
	//Источник синхронизации внешнего сигнала CK_INT
	TIM_PWM->SMCR &=~TIM_SMCR_SMS;		//	SMS=000: Slave mode disabled 
	/* Настройка канала сравнения*/
	// Выбираем режим  PWM mode 2  установкой битов OC2M регистра TIM_PWM->CCMR2 [2:0]  111
	TIM_PWM->CCMR2 |=TIM_CCMR2_OC4M|TIM_CCMR2_OC4PE; 			//Output compare 2 preload enable
	
	TIM_PWM->CCER&=~TIM_CCER_CC4P;  //OC4 active high.
	TIM_PWM->CR1|=TIM_CR1_CEN;			//включение таймера
	//TIM_PWM->CCER|=TIM_CCER_CC4E;		//OC4 signal is output on the corresponding output pin.
	//backlight=BACKLIGHT_ON;
	
/* Вывод TIM_PWM_CH1 PWM для удвоителя напряжения	
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(LCD_PORT_CMD,&GPIO_InitStruct);*/
/*Настроим СР1 таймера TIM_PWM для PWM удвоителя напряжения */
	//Зададим коэффициент деления для CK_CNT=CK_PSC/(PSC[15:0]+1)  48000000/(4799+1) =10000Hz
	//TIM_PWM->PSC=4799;
	//TIM_PWM->ARR=100;			Эти регистры настроили в LCD_Driver для управления backlight LCD
	//TIM_PWM->CCR1=50;
	//Источник синхронизации внешнего сигнала CK_INT
	//TIM_PWM->SMCR &=~TIM_SMCR_SMS;		//	SMS=000: Slave mode disabled       LCD_Driver
	/* Настройка канала сравнения*/
	// Выбираем режим  PWM mode 2  установкой битов OC1M регистра TIM2->CCMR1 [2:0]  111
	//TIM_PWM->CCMR1 |=TIM_CCMR1_OC1M|TIM_CCMR1_OC1PE; 			//Output compare 1 preload enable
	
	//TIM_PWM->CCER&=~TIM_CCER_CC1P;   //OC1 active high.
	//TIM_PWM->CR1|=TIM_CR1_CEN;		//включение таймера                        LCD_Driver
	//TIM_PWM->CCER|=TIM_CCER_CC1E;		//OC1 signal is output on the corresponding output pin.



		//USB_Disconnect_Config();
		//USB_Cable_Config(DISABLE);	
}
#if 0
/****************************************************************/
/* 															Инициализация RTC								*/																					
/****************************************************************/
void RTC_Init(void){
		RCC->APB1ENR |= RCC_APB1ENR_BKPEN | RCC_APB1ENR_PWREN;								// Включим тактирование BKP и PWR интерфейса  
		PWR_BackupAccessCmd(ENABLE);																					// Разрешим доступ к бэкап домену установкой бита DBP
		if ((RCC->BDCR & RCC_BDCR_LSERDY) != RCC_BDCR_LSERDY)  							// Проверим если часы запущены то выход из инициализации
		// Если часы не включены проинициализируем их и загрузим значения делителя и начальное значение счетчика
			{	
				RCC->BDCR |= RCC_BDCR_BDRST;																	// Установим сброс на BKP  домен 
				RCC->BDCR &=~ RCC_BDCR_BDRST;																	// Снимим сброс с BKP  домена 
				RCC->BDCR |= RCC_BDCR_LSEON;																	// Включим 	усилитель кварца 32768		
																																			// Дождемся установки бита готовности LSERDY
				while((RCC->BDCR&RCC_BDCR_LSERDY)!=RCC_BDCR_LSERDY){}	/*Ждем установки LSERDY или примерно 1.5 сек*/
				RCC->BDCR |= RCC_BDCR_RTCSEL_LSE;															/*Если флаг установился настраиваем RTC*/	
				RCC->BDCR|=RCC_BDCR_RTCEN;
				
				while ((RTC->CRL & RTC_CRL_RTOFF)!= RTC_CRL_RTOFF){}					// Дождемся готовности часов для записи
				RTC->CRL |= RTC_CRL_CNF;																			// Установим флаг CNF для входа в режим конфигурации
				RTC->PRLL = 0x7fff;																						// Загрузим в предделитель число 0x7fff 
				RTC->CRL &=~ RTC_CRL_CNF;																		
				while ((RTC->CRL & RTC_CRL_RTOFF)!= RTC_CRL_RTOFF){}					// Дождемся готовности часов для записи
				
				RTC->CRL &=~RTC_CRL_RSF;																			// После запуска часов сбросим флаг RSF	
				while ((RTC->CRL &RTC_CRL_RSF)!=RTC_CRL_RSF){}								// И дождемся его аппаратной установки
				//while ((RTC->CRL & RTC_CRL_RTOFF)!= RTC_CRL_RTOFF){}					// Дождемся готовности часов для записи
				//RTC->CRL |= RTC_CRL_CNF;																			// Установим флаг CNF для входа в режим конфигурации
				//RTC->ALRH =BKP->DR1;
				//RTC->ALRL =BKP->DR2;
				//RTC->CRL &=~ RTC_CRL_CNF;																		
				//while ((RTC->CRL & RTC_CRL_RTOFF)!= RTC_CRL_RTOFF){}					// Дождемся готовности часов для записи
			}	
			RTC->CRH |= RTC_CRH_SECIE;				// Разрешим прерывание от секундных импульсов в модуле RTC SECIE
}
#endif
/****************************************************************/
/* 															Приветствие на экране						*/																					
/****************************************************************/
void Wellcom(void){

	LCD_PutTextAT(text_7kts_USB,0x04);
	LCD_PutTextAT(text_adapter,0x45);
	Delay (2500000);
	LCD_Clear();

}

/****************************************************************/
/* 															Инициализация SD карты					*/																					
/****************************************************************/
FRESULT SD_Card_Selftest(void){
	//DWORD freeclust,freesect,totsect;	
	//FATFS *fatfs;
	//uint32_t temp;
	uint32_t *sn;
	char label[20];
	FRESULT res;
	
	SPI_SD_GETCSD();
			
	res=f_mount (0,&fs);
	res=f_getlabel("0:",label,(DWORD*)sn);
	if(res==FR_NO_FILESYSTEM)
	{
		f_mkfs(0,0,4096);
		res=f_getlabel("0:",label,(DWORD*)sn);
	}
	if((label[0]!='7')&&(label[1]!='K')&&(label[2]!='T'))
		f_setlabel("7KTS_USB");
	
	//fatfs=&fs;
	//res=f_getfree("0:",&freeclust,&fatfs);
	
	sdcard_reinitial();		////Програмный сброс после операций чтения записи и переинициализация для снижения потребления 
	
	/*freesect=(freeclust*fatfs->csize)/2048;
	totsect=((fatfs->n_fatent-2)*fatfs->csize)/2048;
	LCD_PutTextAT(text_SDCard,0x02);	
	LCD_PutTextAT(text_Insert,0x42);
	Delay (2500000);
	LCD_Clear();
			
	LCD_PutTextAT(text_Vsego_MB,0x00);
	LCD_PutTextAT(text_Svobodno_MB,0x40);		
	LCD_PutTextAT("MB" ,0xE);
	LCD_PutTextAT("MB" ,0x4E);
			
	temp=Hex2Dec(totsect);
	LCD_PutValueAT ((uint8_t*)&temp,0x09,4);//LCD_PutValueAT ((uint8_t*)&temp,0x09,4);
	temp=Hex2Dec(freesect);
	LCD_PutValueAT ((uint8_t*)&temp,0x49,4);	
	Delay (2500000);
	LCD_Clear();	*/
	return res;
}
#if 0
/****************************************************************/
/*						Функция Вычисления даты по значению секунд 				*/
/****************************************************************/
void DateCalc(void){			 
	int day=1, year=0,month=0, i,leap;										// с регистра RTC->CNT
	day+=(RTC->CNTH<<16 |RTC->CNTL)/86400;
	// Сначала вычислим год. Сохраним значение в ячейке year	
		while (day>=366)
			{		
				day-=366;
				year++;
				for (i=0;i<3;i++)
					{	
						if (day<=365)
						break;
						day-=365;
						year++;
					}
			 }
			 // Проверим является ли полученный год високосным	
	 if(year%4!=0)
			leap=0;
		else
			leap=1;
		// Остаток дней от вычисления года переведем в месяцы и дни		
		for (i=0; day>days[leap][i];i++)
				{
				month++;
				day-=days[leap][i];
				}
				// Запишем все в соответ. ячейки памяти предварительно приведя к виду удобному для индикации 		
		Time.year=year;
		Time.month=month;
		Time.day=day;
		
}

#endif
/****************************************************************/
/*															Настройка USART_3								*/
/****************************************************************/
void USART1_Init(void){
		GPIO_InitTypeDef GPIO_InitStruct;
/*	USART1_TX PA9   		*/	
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_OD;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);	
/* USART1_RX PA10				*/
// Input floating
	
	/*	9600=48000000/(16*USARTDIV) USARTDIV=312.5  156=0x138   0.5*16=8		*/
	//USART1->BRR=0x1388;
	USART1->CR1&=~(USART_CR1_M|USART_CR1_PCE);	
	USART1->CR2|=USART_CR2_STOP_1;
	USART1->CR3 |=USART_CR3_DMAT|USART_CR3_DMAR;
	
		//USART1_TX DMA1_Channel4
	DMA1_Channel4->CCR |=DMA_CCR4_MINC
												|DMA_CCR4_DIR;								// derection read from memory
	DMA1_Channel4->CPAR=(uint32_t)&USART1->DR;
	
	DMA1_Channel5->CCR |=DMA_CCR5_MINC;
												//|DMA_CCR4_DIR							// derection read from peripheral
	DMA1_Channel5->CPAR=(uint32_t)&USART1->DR;
	//USART1->CR1|=USART_CR1_UE;
	//USART3->CR1|=USART_CR1_TE|USART_CR1_RE;
}

void ADC1_Init (void){
		GPIO_InitTypeDef GPIO_InitStruct;
		DMA_InitTypeDef  DMA_InitStruct;
//-----------------------------------------------------------------/
//				Включим тактирование АЦП1 и делитель для АЦП1 на 6Мгц /	
//-----------------------------------------------------------------/	
		RCC_ADCCLKConfig(RCC_PCLK2_Div8);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
				
// GPIOx config ------------------------------------------------------
		/*PB0=ADC_IN8  */
		GPIO_InitStruct.GPIO_Pin=GPIO_Pin_0;
		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AIN;
		GPIO_Init(GPIOB, &GPIO_InitStruct);
	
		/*	PB2=ADC_SWITCH_PIN 	*/
		GPIO_InitStruct.GPIO_Pin=ADC_SWITCH_PIN;
		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_OD;
		GPIO_Init(ADC_SWITCH_PORT, &GPIO_InitStruct);
		
		ADC_SWITCH_PORT->BSRR=ADC_SWITCH_ON;
		
// DMA1 Channel1------------------------------------------------------
		DMA_InitStruct.DMA_BufferSize=4;			
		DMA_InitStruct.DMA_DIR=DMA_DIR_PeripheralSRC;
		DMA_InitStruct.DMA_M2M=DMA_M2M_Disable;
		DMA_InitStruct.DMA_MemoryBaseAddr=(uint32_t)&ADC1Buff;
		DMA_InitStruct.DMA_MemoryDataSize=DMA_MemoryDataSize_HalfWord;
		DMA_InitStruct.DMA_MemoryInc=DMA_MemoryInc_Enable;
		DMA_InitStruct.DMA_Mode=DMA_Mode_Circular;
		DMA_InitStruct.DMA_PeripheralBaseAddr=(uint32_t)&ADC1->DR;
		DMA_InitStruct.DMA_PeripheralDataSize=DMA_PeripheralDataSize_HalfWord;
		DMA_InitStruct.DMA_PeripheralInc=DMA_PeripheralInc_Disable;
		DMA_InitStruct.DMA_Priority=DMA_Priority_Low;
		DMA_Init(DMA1_Channel1, &DMA_InitStruct);
/*Enable DMA1*/
		DMA_Cmd(DMA1_Channel1, ENABLE);

//ADC1 config --------------------------------------------------------
		ADC1->CR1|=ADC_CR1_EOCIE						/*	 EOC interrupt enabled.	*/
							/*|ADC_CR1_SCAN*/;						/*Scan mode */
	
		ADC1->CR2|=ADC_CR2_EXTTRIG					/* 	Conversion on external event enabled */
							 |ADC_CR2_EXTSEL					/* 	EXTSEL[2:0] 111: SWSTART  */
							 |ADC_CR2_DMA;						/*	DMA enable */
		
		ADC1->SMPR2 |=ADC_SMPR2_SMP8;			/*	SMPx[2:0] 111= 239.5 cycles	*/
		
		ADC1->SQR1|=ADC_SQR1_L_0						/*	0011: 4 conversions 	*/
								|ADC_SQR1_L_1;
								
		ADC1->SQR3|=ADC_SQR3_SQ1_3
							 |ADC_SQR3_SQ2_3
							 |ADC_SQR3_SQ3_3
							 |ADC_SQR3_SQ4_3;
		
		
		ADC1->CR1|=ADC_CR1_SCAN;
		ADC1->CR2|=ADC_CR2_ADON;
		Delay(100000);
		ADC1->CR2|=ADC_CR2_RSTCAL;
		while(ADC1->CR2&ADC_CR2_RSTCAL) {};		
		ADC1->CR2|=ADC_CR2_CAL;
		while(ADC1->CR2&ADC_CR2_CAL) {};
		
		ADC1->CR2|=ADC_CR2_SWSTART;
		NVIC_SetPriority(ADC1_2_IRQn,2);					//Приоритет ADC1=2
		NVIC_EnableIRQ(ADC1_2_IRQn);	
		/*ADC_InitStruct.ADC_ContinuousConvMode=DISABLE;
		ADC_InitStruct.ADC_ScanConvMode=DISABLE;
		ADC_InitStruct.ADC_Mode=ADC_Mode_Independent;
		ADC_InitStruct.ADC_DataAlign=ADC_DataAlign_Right;
		ADC_InitStruct.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//ADC_ExternalTrigConv_T4_CC4;
		ADC_InitStruct.ADC_NbrOfChannel=4;	    											// Задаем количество каналов для регулярного преобразования
		ADC_Init(ADC1,&ADC_InitStruct);*/
		//ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
		//ADC_DMACmd(ADC1, ENABLE);	
		/*ADC_AutoInjectedConvCmd(ADC1, DISABLE);
		ADC_InjectedDiscModeCmd(ADC1, ENABLE);
		ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_None);
		ADC_ExternalTrigInjectedConvCmd(ADC1, DISABLE);
		ADC_InjectedSequencerLengthConfig(ADC1,1);
		ADC_InjectedChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_239Cycles5);*/
		/*ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 1, ADC_SampleTime_55Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 2, ADC_SampleTime_55Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 3, ADC_SampleTime_55Cycles5);
		ADC_RegularChannelConfig(ADC1, ADC_Channel_8, 4, ADC_SampleTime_55Cycles5);*/
		//ADC_ExternalTrigConvCmd(ADC1, ENABLE);
	}
/*
*/
void Change_Freq(_7KTS_SPEED speed){
	
	if(speed==HSE)
	{
		 /* Select HSE as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_HSE; 
		/* Wait till HSE is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x04){ }
		
		/* Disable Prefetch Buffer */
    FLASH->ACR &= ~FLASH_ACR_PRFTBE;
		/* Flash 0 wait state */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
		 /* Disable PLL */
    RCC->CR &= ~RCC_CR_PLLON;
		TIM2->PSC=7999;
		TIM_PWM->PSC=79;
	}
	else
	{
		
		/* Enable Prefetch Buffer */
    FLASH->ACR |= FLASH_ACR_PRFTBE;
		/* Flash 1 wait state */
    FLASH->ACR &= (uint32_t)((uint32_t)~FLASH_ACR_LATENCY);
    FLASH->ACR |= (uint32_t)FLASH_ACR_LATENCY_1;    
	
		 /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;
    /* Wait till PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0) { }
    /* Select PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;    
   /* Wait till PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)0x08){ }
		TIM2->PSC=47999;
		TIM_PWM->PSC=479;
	}
}	
/*
*
*/
