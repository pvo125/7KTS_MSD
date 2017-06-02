#include <stm32f10x.h>
#include "LCD_Driver.h"
//




void Delay (uint32_t ticks){														// Функция задержки на таймере SysTick
	SysTick->LOAD=(ticks*6);
	SysTick->VAL=0;
	while(!(SysTick->CTRL &SysTick_CTRL_COUNTFLAG_Msk)){}	
	
}
 void LCD_Init (void){
	 
	GPIO_InitTypeDef GPIO_InitStruct;
	/*Настройка выводов для управления LCD */

//  линиии DATA 
	GPIO_InitStruct.GPIO_Pin=LCD_D0|LCD_D1|LCD_D2|LCD_D3|
													 LCD_D4|LCD_D5|LCD_D6|LCD_D7;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_PP;
  GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(LCD_PORT_DATA,&GPIO_InitStruct);
	//	Линия R/W
	//GPIO_InitStruct.GPIO_Pin=LCD_RW;
	//GPIO_Init(LCD_PORT_RW,&GPIO_InitStruct);
	/*Выставим на этой линии лог. ноль. Будем только писать в LCD*/
	//LCD_PORT_RW->BSRR=RW_Low;

//  Линии A0,E 
	GPIO_InitStruct.GPIO_Pin=LCD_A0|LCD_E;
	GPIO_Init(LCD_PORT_CMD,&GPIO_InitStruct);

	 // Пауза 15ms
	Delay(15000);																		
		
	LCD_PORT_CMD->BSRR =A0_Low;													// A0=0 RW=0 
	LCD_PORT_CMD->BSRR =E_High;													// E=1
	LCD_PORT_DATA->ODR=0xA000|0x30;											// A0 R/W DB7 DB6 DB5 DB4 
	//Delay(1);																		// 0  0   0   0   1   1	
	LCD_PORT_CMD->BSRR =E_Low;														// E=0						
	Delay(40);																		// Пауза 40us
	
	LCD_PORT_CMD->BSRR =A0_Low;													// A0=0 RW=0 
	LCD_PORT_CMD->BSRR =E_High;													// E=1
	LCD_PORT_DATA->ODR=0xA000|0x30;											// A0 R/W DB7 DB6 DB5 DB4 
	//Delay(1);																		// 0  0   0   0   1   1	
	LCD_PORT_CMD->BSRR =E_Low;														// E=0						
	Delay(40);																		// Пауза 40us
	
	LCD_PORT_CMD->BSRR =A0_Low;													// A0=0 RW=0 
	LCD_PORT_CMD->BSRR =E_High;													// E=1
	LCD_PORT_DATA->ODR=0xA000|0x30;											// A0 R/W DB7 DB6 DB5 DB4 
	//Delay(1);																		// 0  0   0   0   1   1	
	LCD_PORT_CMD->BSRR =E_Low;														// E=0						
	Delay(40);																		// Пауза 40us
	
	// Function set установка параметров
	LCD_PORT_CMD->BSRR =A0_Low;														// A0=0 RW=0
	LCD_PORT_CMD->BSRR =E_High;														// E=1
	LCD_PORT_DATA->ODR =0xA000|0x3B;											// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//Delay(1);																							// 0  0   0   0   1   1		1		0		P		0
	LCD_PORT_CMD->BSRR =E_Low;															// E=0						
	Delay(40);
	
	// Display OFF control
	LCD_PORT_CMD->BSRR =A0_Low;														// A0=0 RW=0
	LCD_PORT_CMD->BSRR =E_High;														// E=1
	LCD_PORT_DATA->ODR =0xA000|0x08;											// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//Delay(1);																							// 0  0   0   0   0   0		1		0		0		0
	LCD_PORT_CMD->BSRR =E_Low;															// E=0						
	Delay(40);
	
	// Clear Display 
	LCD_PORT_CMD->BSRR =A0_Low;														// A0=0 RW=0
	LCD_PORT_CMD->BSRR =E_High;														// E=1
	LCD_PORT_DATA->ODR =0xA000|0x01;											// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//Delay(1);																			// 0  0   0   0   0   0		0		0		0		1
	LCD_PORT_CMD->BSRR =E_Low;															// E=0						
	Delay(1700);

// Entry Mode Set 
//ID=1 вправо
//SH=0
	LCD_PORT_CMD->BSRR =A0_Low;														// A0=0 RW=0
	LCD_PORT_CMD->BSRR =E_High;														// E=1
	LCD_PORT_DATA->ODR =0xA000|0x06;											// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//Delay(1);																			// 0  0   0   0   0   0		0		1		1		0
	LCD_PORT_CMD->BSRR =E_Low;															// E=0						
	Delay(40);

// Display ON control
	// Display OFF control
	LCD_PORT_CMD->BSRR =A0_Low;															// A0=0 RW=0
	LCD_PORT_CMD->BSRR =E_High;															// E=1
	LCD_PORT_DATA->ODR =0xA000|0x0C;												// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//Delay(1);																				// 0  0   0   0   0   0		1		1		0		0
	LCD_PORT_CMD->BSRR =E_Low;																// E=0						
	Delay(40);
}



//
void LCD_Clear (void){
	// Clear Display 
	
	LCD_PORT_CMD->BSRR =A0_Low;															// A0=0 RW=0
	LCD_PORT_CMD->BSRR =E_High;															// E=1
	LCD_PORT_DATA->ODR =0xA000|0x01;												// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//Delay(1);																				// 0  0   0   0   0   0		0		0		0		1
	LCD_PORT_CMD->BSRR =E_Low;																// E=0						
	Delay(1700);
	
}



// Функция установки адреса pos для последующего вывода символа
 void Set_DDRAM_Address (uint8_t pos){
	
	 
	LCD_PORT_CMD->BSRR =A0_Low;														// A0=0 RW=0
	LCD_PORT_CMD->BSRR =E_High;														// E=1
	LCD_PORT_DATA->ODR = 0xA000|(pos|0x80);								// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
	//Delay(1);																			// 0  0   1   a   a   a		a		a		a		a
	LCD_PORT_CMD->BSRR =E_Low;															// E=0						
	Delay(40);
		
}



//Функция сдвига курсора на одну позицию влево RL=0 вправо RL=1  
 void Shift_cursor (uint8_t RL){	
	
		LCD_PORT_CMD->BSRR =A0_Low;													// A0=0 RW=0
		LCD_PORT_CMD->BSRR =E_High;													// E=1
		LCD_PORT_DATA->ODR = 0xA000|(0x10|(RL<<2));					// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
		//Delay(1);																		// 0  0   0   0  	0   1		0		RL	x		x
		LCD_PORT_CMD->BSRR =E_Low;														// E=0						
		Delay(40);
	
}



// Функция стирания символа на n позиций влево или вправо
 void Backspace_cursor(uint8_t RL,uint8_t n){
		uint8_t i;
	 for (i=0;i<n;i++)
	 {
		Shift_cursor (RL);
		
		LCD_PORT_CMD->BSRR =A0_Low;													// A0=0 RW=0
		LCD_PORT_CMD->BSRR =E_High;													// E=1
		LCD_PORT_DATA->ODR = 0xA000|(0x20);									// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
		//Delay(1);																		// 0  0   d   d   d   d		d		d		d		d
		LCD_PORT_CMD->BSRR =E_Low;														// E=0						
		Delay(40);								
																	
		Shift_cursor (RL);
	 }
}





// Функция вывода символов c массива текста buff[] в позицию pos
 void LCD_PutTextAT(const char *buff,uint8_t pos){
	uint8_t i;
	
	Set_DDRAM_Address (pos);
	// Выводим символы 
	for (i=0; buff[i] !='\0'; i++)
	 {
		LCD_PORT_CMD->BSRR =A0_High;											// A0=1 RW=0
		LCD_PORT_CMD->BSRR =E_High;											// E=1
		LCD_PORT_DATA->ODR = 0xA000|buff[i];											// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
		//Delay(1);																// 1  0   d   d   d   d		d		d		d		d
		LCD_PORT_CMD->BSRR =E_Low;												// E=0						
		Delay(40);					
		}
}





// Функция вывода n-символов в позицию pos с ячейки памяти  *addr
 void LCD_PutValueAT(uint8_t *addr,uint8_t pos,uint8_t n){
	uint8_t i;
	/*NVIC->ICER[0]=NVIC_ICER_CLRENA_7|
								NVIC_ICER_CLRENA_8|
								NVIC_ICER_CLRENA_9|
								NVIC_ICER_CLRENA_29;*/
		
	Set_DDRAM_Address (pos);
// Выводим символ с адреса addr в позицию курсора
	for (i=0; i<n; i++)
			{
			LCD_PORT_CMD->BSRR =A0_High;										// A0=1 RW=0
			LCD_PORT_CMD->BSRR =E_High;											// E=1
			LCD_PORT_DATA->ODR = 0xA000|(*addr);						// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
			//Delay(1);																			// 1  0   d   d   d   d		d		d		d		d
			LCD_PORT_CMD->BSRR =E_Low;											// E=0						
			Delay(40);			
			addr++;
			}
	/*NVIC->ISER[0]=NVIC_ISER_SETENA_7|
								NVIC_ISER_SETENA_8|
								NVIC_ISER_SETENA_9|
								NVIC_ISER_SETENA_29;*/
}

//
void LCD_PutCharAT (char simvol ,uint8_t pos){
	/*NVIC->ICER[0]=NVIC_ICER_CLRENA_7|
								NVIC_ICER_CLRENA_8|
								NVIC_ICER_CLRENA_9|
								NVIC_ICER_CLRENA_29;*/
	
	Set_DDRAM_Address (pos);
	// Выводим символ simvol в позицию курсора
			LCD_PORT_CMD->BSRR =A0_High;															// A0=1 RW=0
			LCD_PORT_CMD->BSRR =E_High;															// E=1
			LCD_PORT_DATA->ODR = 0xA000|simvol;															// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
			//Delay(1);																				// 1  0   d   d   d   d		d		d		d		d
			LCD_PORT_CMD->BSRR =E_Low;																// E=0						
			Delay(40);			
			/*NVIC->ISER[0]=NVIC_ISER_SETENA_7|
										NVIC_ISER_SETENA_8|
										NVIC_ISER_SETENA_9|
										NVIC_ISER_SETENA_29;*/
}



//Функция вывода символа в текущую позицию курсора 
 void LCD_PutChar(char simvol){
			
			LCD_PORT_CMD->BSRR =A0_High;															// A0=1 RW=0
			LCD_PORT_CMD->BSRR =E_High;															// E=1
			LCD_PORT_DATA->ODR = 0xA000|simvol;															// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
			//Delay(1);																				// 1  0   d   d   d   d		d		d		d		d
			LCD_PORT_CMD->BSRR =E_Low;																// E=0						
			Delay(40);			
}



// Функция изменения типа отображения курсора
void Cursor_type(uint32_t type){
	
	LCD_PORT_CMD->BSRR =A0_High;				// A0=0 
	LCD_PORT_CMD->BSRR =E_High;											// E=1
	LCD_PORT_DATA->BSRR =0x0F000000;
	LCD_PORT_CMD->BSRR =E_Low;											// E=0						// A0 R/W DB7 DB6 DB5 DB4
	Delay(40);
}

// Функция вывода времени 
void LCD_PutTimeAT(uint8_t *time,uint8_t pos){
		uint8_t i;
		Set_DDRAM_Address (pos);
		for (i=0; i<2; i++)
			{
			LCD_PORT_CMD->BSRR =A0_High;										// A0=1 RW=0
			LCD_PORT_CMD->BSRR =E_High;											// E=1
			LCD_PORT_DATA->ODR = 0xA000|(*time/10)|0x30;						// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
			//Delay(1);																			// 1  0   d   d   d   d		d		d		d		d
			LCD_PORT_CMD->BSRR =E_Low;											// E=0						
			Delay(40);			
			
			LCD_PORT_CMD->BSRR =A0_High;										// A0=1 RW=0
			LCD_PORT_CMD->BSRR =E_High;											// E=1
			LCD_PORT_DATA->ODR = 0xA000|(*time%10)|0x30;						// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
			//Delay(1);																			// 1  0   d   d   d   d		d		d		d		d
			LCD_PORT_CMD->BSRR =E_Low;											// E=0						
			Delay(40);
			if(i<1)	
				LCD_PutChar(0x3A);
				
			time+=2;
			}
}

// Функция вывода даты
void LCD_PutDateAT(uint8_t *date,uint8_t pos){
		uint8_t i;
		Set_DDRAM_Address (pos);
		for (i=0; i<3; i++)
			{
			LCD_PORT_CMD->BSRR =A0_High;										// A0=1 RW=0
			LCD_PORT_CMD->BSRR =E_High;											// E=1
			LCD_PORT_DATA->ODR = 0xA000|(*date/10)|0x30;						// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
			//Delay(1);																			// 1  0   d   d   d   d		d		d		d		d
			LCD_PORT_CMD->BSRR =E_Low;											// E=0						
			Delay(40);			
			
			LCD_PORT_CMD->BSRR =A0_High;										// A0=1 RW=0
			LCD_PORT_CMD->BSRR =E_High;											// E=1
			LCD_PORT_DATA->ODR = 0xA000|(*date%10)|0x30;						// A0 R/W DB7 DB6 DB5 DB4 DB3 DB2 DB1 DB0
			//Delay(1);																			// 1  0   d   d   d   d		d		d		d		d
			LCD_PORT_CMD->BSRR =E_Low;											// E=0						
			Delay(40);
			if(i<2)	
				LCD_PutChar(0x2E);
				
			date+=2;
			}
}

uint32_t Hex2Dec(uint16_t hex){
	uint32_t temp;
	uint32_t dec;
	
	dec=(hex/1000)|0x30;
	dec|=(((hex%1000)/100|0x30)<<8);
	temp=(hex%1000)%100;
	
	dec|=(((temp/10)|0x30)<<16);
	dec|=((temp%10)|0X30)<<24;	
	return dec;
}

/**/

const char text_adapter[]={0xC0,0xE4,0xE0,0xEF,0xF2,0xE5,0xF0,'\0'}; 												 //Адаптер
const char text_7kts_USB[]="7KTS-USB";
const char text_SDCard[]={'S','D',' ',0xEA,0xE0,0xF0,0xF2,0xE0,'\0'};   										// SD карта
const char text_Insert[]={0xE2,0xF1,0xF2,0xE0,0xE2,0xEB,0xE5,0xED,0xE0,'\0'};  							// вставлена
const char text_Vsego_MB[]={0xC2,0xF1,0xE5,0xE3,0xEE,'\0'};  																// Всего 
const char text_Svobodno_MB[]={0xD1,0xE2,0xEE,0xE1,0xEE,0xE4,0xED,0xEE,'\0'}; 		 					// Свободно
const char text_SDCard_Error[]={0xCE,0xF8,0xE8,0xE1,0xEA,0xE0,' ','S','D',' ',0xEA,0xE0,0xF0,0xF2,0xFB,'!','\0'};// Ошибка SD карты!
const char text_NoSDCard[]={0xCD,0xE5,0xF2,' ','S','D',' ',0xEA,0xE0,0xF0,0xF2,0xFB,'!','\0'};// Нет SD карты!!!

const char text_Port[]={0xEF,0xEE,0xF0,0xF2,'\0'};	//порт
const char text_CHARGE_mode[]={0xC7,0xE0,0xF0,0xFF,0xE4,'\0'};						//Заряд
const char text_Read_7KT[]={0xD7,0xE8,0xF2,0xE0,0xF2,0xFC,'\0'};					// Читать
const char text_Connect[]={0xD1,0xE2,0xFF,0xE7,0xFC,'.','.','.','\0'};		// Связь...
const char text_Error[]={0xCE,0xF8,0xE8,0xE1,0xEA,0xE0,'\0'};							// Ошибка 
const char text_Connecting[]={0xF1,0xE2,0xFF,0xE7,0xE8,'!','\0'};					// связи !

const char text_DIR[]={0xEA,0xE0,0xF2,0xE0,0xEB,0xEE,0xE3,0xE0,'\0'};		  //каталога
const char text_File[]={0xF4,0xE0,0xE9,0xEB,0xE0,'\0'};			  						// файла
const char text_Reading__[]={0xD7,0xF2,0xE5,0xED,0xE8,0xE5,'.','.','\0'};	// Чтение..
const char text_Readed_OK[]={0xF1,0xF7,0xE8,0xF2,0xE0,0xED,'!','\0'};			// считан !
const char text_VAKB[]={0xC1,0xE0,0xF2,0xE0,0xF0,0xE5,0xFF,'\0'};					// Батарея
const char text_Low[]={0xCD,0xE8,0xE7,0xEA,0xE8,0xC9,'\0'};								// Низкий


