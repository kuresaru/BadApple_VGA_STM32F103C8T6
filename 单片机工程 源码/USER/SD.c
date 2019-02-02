#include "stm32f10x.h"
#include "delay.h"
#include "SD.h"

#define SPIBUSY SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) != RESET
#define SPI_HIGHSPEED_PRESCALER SPI_BaudRatePrescaler_2 //有时候太高了会卡住...

#define WAIT_COUNT 200
#define WAIT_COUNT_HIGH 30000

struct SD_Info {
	enum SD_Type {
		UNINITIALIZED=0, SD_1, SD_2, SD_HC
	} SD_Type;
} SD_Info;

void SD_InitSPI() {
	SPI_InitTypeDef spii;
	GPIO_InitTypeDef ioi;
	
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	
	ioi.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15; // CLK/MOSI
	ioi.GPIO_Speed = GPIO_Speed_50MHz;
	ioi.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &ioi);
	
	ioi.GPIO_Pin = GPIO_Pin_14; // MISO
	ioi.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &ioi);
	
	ioi.GPIO_Pin = GPIO_Pin_12; // CS
	ioi.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &ioi);
	
	spii.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spii.SPI_Mode = SPI_Mode_Master;
	spii.SPI_DataSize = SPI_DataSize_8b;
	spii.SPI_CPOL = SPI_CPOL_Low; //时钟悬空低
	spii.SPI_CPHA = SPI_CPHA_1Edge; //第一个时钟沿
	spii.SPI_NSS = SPI_NSS_Soft;
	spii.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256; // 72MHz/256=281.25KHz SD卡初始化时需要低速
	spii.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init(SPI2, &spii);
	
	SPI_Cmd(SPI2, ENABLE);
}

u8 SD_RW(u8 dat) {
	SPI_I2S_SendData(SPI2, dat);
	while (SPIBUSY);
	return SPI_I2S_ReceiveData(SPI2) & 0xFF;
}

u8 SD_SendNOP() {
	return SD_RW(0xFF);
}

u8 SD_SendData(u8 cmd, u32 arg, u8 crc) {
	u8 tmp, wait = WAIT_COUNT;
	SD_SendNOP();
	SD_RW(cmd);
	SD_RW((arg >> 24) & 0xFF);
	SD_RW((arg >> 16) & 0xFF);
	SD_RW((arg >> 8) & 0xFF);
	SD_RW(arg & 0xFF);
	SD_RW(crc);
	do {
		tmp = SD_SendNOP();
	} while (tmp == 0xFF && wait--);
	return tmp;
}

u8 SD_InitCard() {
	u8 tmp, wait;
	SD_ResetCS;
	Delay_ms(1);
	SD_SetCS;
	Delay_ms(1);
	for (tmp = 0; tmp < 16; tmp++) { //初始化时钟
		while (SPIBUSY);
		SPI_I2S_SendData(SPI2, 0xFF);
	}
	Delay_ms(1);
	SD_ResetCS;
	tmp = SD_SendData(0x40, 0x00000000, 0x95); //CMD0
	if (tmp != 0x01) {
		SD_SetCS;
		return 1;
	}
	tmp = SD_SendData(0x48, 0x000001AA, 0x87); //CMD8
	if (tmp == 0x01) {
		//SD2.0
		SD_Info.SD_Type = SD_2;
		SD_SendNOP(); //接收完CMD8的4个字节
		SD_SendNOP();
		SD_SendNOP();
		SD_SendNOP();
		wait = WAIT_COUNT;
		do {
			Delay_ms(1);
			SD_SendData(0x77, 0x00000000, 0xFF); //CMD55
			tmp = SD_SendData(0x69, 0x40000000, 0xFF); //ACMD41
		} while (tmp != 0x00 && wait--);
		if (tmp == 0x00) {
			SD_SendData(0x7A, 0x00000000, 0xFF); //CMD58
			tmp = SD_SendNOP();
			SD_SendNOP();
			SD_SendNOP();
			SD_SendNOP();
			if (tmp & 0x40) {
				SD_Info.SD_Type = SD_HC;
			}
		} else {
			SD_SetCS;
			return 1;
		}
	} else if (tmp == 0x05) {
		//SD1.0
		SD_Info.SD_Type = SD_1;
	} else {
		SD_SetCS;
		return 1;
	}
	SD_SetCS;
	
	//Delay_ms(2);
	SPI_InitTypeDef spii;
	
	SPI_Cmd(SPI2, DISABLE);
	spii.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	spii.SPI_Mode = SPI_Mode_Master;
	spii.SPI_DataSize = SPI_DataSize_8b;
	spii.SPI_CPOL = SPI_CPOL_Low; //时钟悬空低
	spii.SPI_CPHA = SPI_CPHA_1Edge; //第一个时钟沿
	spii.SPI_NSS = SPI_NSS_Soft;
	spii.SPI_BaudRatePrescaler = SPI_HIGHSPEED_PRESCALER; //高速模式
	spii.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_Init(SPI2, &spii);
	
	SPI_Cmd(SPI2, ENABLE);
	//Delay_ms(2);
	return 0;
}

u8 SD_StartReadBlock(u32 addr) {
	u8 tmp;
	u16 wait = WAIT_COUNT_HIGH;
	if (SD_Info.SD_Type != SD_HC) { //非HC卡 扇区号转字节号
		addr <<= 9;
	}
	do {
		tmp = SD_SendData(0x51, addr, 0xFF);
	} while (tmp && wait--);
	if (tmp == 0) {
		wait = WAIT_COUNT_HIGH;
		do {
			tmp = SD_SendNOP();
		} while (tmp != 0xFE && wait--);
		if (wait) {
			return 1;
		} else {
			return 0;
		}
	}
	return 0;
}
