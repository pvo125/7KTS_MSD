/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/


#include "diskio.h"		/* FatFs lower layer API */
//#include "usbdisk.h"	/* Example: USB drive control */
//#include "atadrive.h"	/* Example: ATA drive control */
#include "spi_sdcard.h"		/* Example: MMC/SDC contorl */
#include "main.h"
#include "RS232.h"
extern DSTATUS sd_card_init(void);
extern uint16_t SD_SECTOR_SIZE;
extern DEV7KT_TypeDef dev7kt;
extern CSDTypeDef csd;
extern uint8_t CardType;			/* Card type flags */
/* Definitions of physical drive number for each drive */
#define ATA		1	/* Example: Map ATA drive to drive number 0 */
#define MMC		0	/* Example: Map MMC/SD card to drive number 1 */
#define USB		2	/* Example: Map USB drive to drive number 2 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/
DSTATUS disk_status (BYTE pdrv){/* Physical drive nmuber to identify the drive */
	if(pdrv)
		return STA_NOINIT;;
	return Stat; 
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/
DSTATUS disk_initialize(BYTE drv){
	
	if(drv) return STA_NOINIT;
	if (Stat & STA_NODISK) return Stat;	/* No card in the socket */
	
	if(Stat==0) return Stat;
	
	Stat=sd_card_init();
		return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	BYTE count		/* Number of sectors to read */
)
{
	if (pdrv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if(!(CardType&CT_BLOCK))
		sector*=512;
	if(count==1)
		return SPI_SD_ReadBlock (sector,buff);
	else
		return SPI_SD_ReadMultiBlock(sector,buff,count);
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	BYTE count			/* Number of sectors to write */
)
{	
	if (pdrv || !count) return RES_PARERR;
	if (Stat & STA_NOINIT) return RES_NOTRDY;
	if (Stat & STA_PROTECT) return RES_WRPRT;
	if(!(CardType&CT_BLOCK))
		sector*=512;
	if(count==1)
		return SPI_SD_WriteBlock(sector,buff);
	else
		return SPI_SD_WriteMultiBlock(sector,buff,count);
	
	
	
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	
	switch (pdrv) {
	case ATA :
	return res;
	
	case MMC :
		switch(cmd){
			case GET_SECTOR_COUNT :	/* Get number of sectors on the disk (DWORD) */
			*(DWORD*) buff=csd.sector_count;
			break;
			
			case GET_SECTOR_SIZE:
			*(DWORD*) buff=SD_SECTOR_SIZE;
			res=RES_OK;
			break;	
				
			case GET_BLOCK_SIZE:
			*(DWORD*) buff =csd.erase_size;
			res = RES_OK;
			break;
			}	
	
			case USB :
			break;
			
			default:
				res=RES_PARERR;
	}

	return res;
}

DWORD get_fattime (void)
{
		DWORD t;
	
	t=(dev7kt.Year+20)<<25;
	t|=dev7kt.Month<<21;
	t|=dev7kt.Day<<16;
	t|=dev7kt.Hour<<11;
	t|=dev7kt.Minute<<5;
	t|=(dev7kt.Sec/2);
	return t;

}


#endif
