#include "stm32f10x.h"
#include "fs.h"
#include "SD.h"
#include "VGA.h"
#include "delay.h"
#include "serial.h"

extern u32 FILE_BadApple_Length;
extern u32 FILE_BadApple_StartClust;
extern u8 FS_SectorPerClust;
extern u8 FS_SectorPerClust;
extern u32 FS_FatUseSector;
extern u32 FS_FatSector;

u8 nextFrameFlag;

void Player_Play() {
	u32 nextClust;
	u32 rootSector = FS_FatSector + FS_FatUseSector + FS_FatUseSector;
	u8 clustSector;
	u16 i, x = 0, y = 0;
	u32 tmp;
	
	SD_ResetCS;
	Serial_Send(0x7E);
	Serial_Send(0xFF);
	Serial_Send(0x06);
	Serial_Send(0x03);
	Serial_Send(0x00);
	Serial_Send(0x00);
	Serial_Send(0x06);
	Serial_Send(0xFE);
	Serial_Send(0xF2);
	Serial_Send(0xEF);
	for (nextClust = FILE_BadApple_StartClust; nextClust != 0x0FFFFFFF; nextClust = FS_GetNextClust(nextClust)) { // 所有簇
		for (clustSector = 0; clustSector < FS_SectorPerClust; clustSector++) { // 所有扇区
			SD_StartReadBlock(rootSector + ((nextClust - 2) * FS_SectorPerClust) + clustSector); //开始读扇区
			for (i = 0; i < 512; i++) {
				VGA_SetBuf(x, y, SD_SendNOP());
				x++;
				if (x == 50) { //一行完了
					x = 0;
					y++; //x归零 y+1
					if (y == 200) { //一帧完了
						y = 0;
						do
							tmp = SysTick->CTRL;
						while ((tmp & 0x01) && (!(tmp & (1 << 16))));
						SysTick->CTRL = 0x00;
						SysTick->LOAD = 9000 * 80;
						SysTick->CTRL = 0x01;
						SysTick->VAL = 0;
					}
				}
			}
			SD_SendNOP();//丢弃两个字节的CRC
			SD_SendNOP();
		}
	}
}
