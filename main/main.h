
#ifndef __MAIN_H
#define __MAIN_H

#include <stm32f10x.h>
#include "ff.h"
#include "diskio.h"
#include "spi_sdcard.h"

#define WKUP_CALLBACK

#define CHARGE_INDIC_PIN									GPIO_IDR_IDR14
#define CHARGE_INDIC_PORT									GPIOB


#define ADC_SWITCH_PIN										GPIO_Pin_7
#define ADC_SWITCH_PORT										GPIOB
#define ADC_SWITCH_OFF										GPIO_BSRR_BS7	
#define ADC_SWITCH_ON											GPIO_BSRR_BR7

#define SW_POWER_PORT											GPIOB
#define SW_POWER_PIN											GPIO_Pin_8

#define USB_DISCONNECT_PORT               GPIOC  
#define USB_DISCONNECT_PIN                GPIO_Pin_13
#define USB_DETECT_PIN										GPIO_IDR_IDR1
#define USB_DETECT_PORT										GPIOB

typedef struct	
{	
	uint8_t hour;
	uint8_t point_1;
	uint8_t min;
	uint8_t point_2;
	uint8_t sec;
	uint8_t day;
	uint8_t point_3;
	uint8_t month;
	uint8_t point_4;
	uint8_t year;
}TimeAlarm_TypeDef;

typedef enum {
	HSE=0,
	PLL
}_7KTS_SPEED;

typedef enum {
	SLEEP_MODE=0,
	NODISK_MODE,
	SDINIT_MODE,
	BATLOW_MODE,
	RS232_MODE,
	USB_MODE,
	CHARGE_MODE	
}_7KTS_STATE;

typedef enum {
	NODATA_MENU=0,
	READ_MENU,
	BATVIEW_MENU
	//FILEVIEW_MENU,
}_MENU_PUNKT;

typedef enum {
	BACKLIGHT_OFF=0,
	BACKLIGHT_ON
}_BACKLIGHT;

extern _BACKLIGHT volatile backlight;
extern _MENU_PUNKT volatile menu_punkt;

extern volatile DSTATUS Stat;

extern	_7KTS_STATE	volatile _7kts_State;
extern uint8_t	volatile Sleep;
extern uint8_t	volatile Wkup_Ext;
extern uint8_t	volatile refresh;

extern void DateCalc(void);

#endif
