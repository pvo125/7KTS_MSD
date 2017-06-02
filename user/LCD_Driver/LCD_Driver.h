#ifndef __LCD_DRIVER_H
#define __LCD_DRIVER_H

#include <stm32f10x.h>
#define C_NONE	0x0C000300
#define C_			0x0E000100
#define C_BLINK	0x00000000
#define Left  	0x00
#define Right		0x01
//

#define LCD_PORT_DATA	 GPIOA
#define LCD_PORT_CMD	 GPIOB
#define LCD_PORT_LED	 GPIOB

#define LCD_A0  GPIO_Pin_12
#define LCD_E   GPIO_Pin_13

#define LCD_D0  GPIO_Pin_0
#define LCD_D1  GPIO_Pin_1
#define LCD_D2  GPIO_Pin_2
#define LCD_D3  GPIO_Pin_3
#define LCD_D4  GPIO_Pin_4
#define LCD_D5  GPIO_Pin_5
#define LCD_D6  GPIO_Pin_6
#define LCD_D7  GPIO_Pin_7

#define LCD_PWM  GPIO_Pin_9
#define TIM_PWM	 TIM4	
#define RCC_APB1Periph_TIM_PWM RCC_APB1Periph_TIM4

#define A0_High  GPIO_BSRR_BS12
#define A0_Low	 GPIO_BSRR_BR12	
#define E_High	 GPIO_BSRR_BS13
#define E_Low		 GPIO_BSRR_BR13


static uint8_t days [2][13]={
	{0,31,28,31,30,31,30,31,31,30,31,30,31},
	{0,31,29,31,30,31,30,31,31,30,31,30,31}
	};		

extern const char text_adapter[];
extern const char text_7kts_USB[];
extern const char text_SDCard[];
extern const char text_Insert[];	
extern const char text_Vsego_MB[]; 
extern const char text_Svobodno_MB[];
extern const char text_SDCard_Error[];	
extern const char text_NoSDCard[];
extern const char text_Port[];
extern const char text_USBPort_mode[];
extern const char text_CHARGE_mode[];	
extern const char text_Read_7KT[];	
extern const char text_Connect[];
extern const char text_Connecting[];
extern const char text_Error[];
extern const char text_DIR[];	
extern const char text_File[];
extern const char text_Reading__[];
extern const char text_Error_crc[];	
extern const char text_Readed_OK[];
extern const char text_VAKB[];	
extern const char text_Low[];
	
	/*const uint8_t Set_Alarm[]="ALARM";
const uint8_t GoodBye[]="GoodBye!";
const uint8_t Power[]="Power";
const uint8_t Method_Control1[]="Method Phase";
const uint8_t Method_Control2[]="       Brezen";
const uint8_t Enable_Method []="Enable method";
const uint8_t Phase_Method_Enabled[]="Method Enabled";
const uint8_t Phase_Method_Disabled[]="Method Disabled";*/
	
 void LCD_Init(void);
 void LCD_Clear(void);
 void LCD_DisplayOFF(void);
 void Delay (uint32_t c);
 void Set_DDRAM_Address(uint8_t pos); 
 void LCD_PutTextAT(const char *buff,uint8_t pos);
 void LCD_PutValueAT (uint8_t *addr,uint8_t pos,uint8_t n);
 void LCD_PutCharAT(char simvol ,uint8_t pos);
 void Backspace_cursor(uint8_t RL,uint8_t n);
 void LCD_PutChar(char simvol);
 void Cursor_type (uint32_t type);
 void Shift_cursor (uint8_t RL);
 uint32_t Hex2Dec(uint16_t hex);
 void LCD_PutTimeAT(uint8_t *time,uint8_t pos);
 void LCD_PutDateAT(uint8_t *date,uint8_t pos);
#endif

