#include "stm32f10x.h"
#include "VGA.h"
#include "SD.h"
#include "fs.h"
#include "player.h"
#include "serial.h"

#define SetTest1 GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define SetTest2 GPIO_SetBits(GPIOA, GPIO_Pin_5)
#define ResetTest1 GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define ResetTest2 GPIO_ResetBits(GPIOA, GPIO_Pin_5)

#define BTN GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)

int main() {
	//初始化结构体定义
	GPIO_InitTypeDef GPIO_InitStructure;
	
	//开时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	//IO接口
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	Serial_Init();
	VGA_Init();
	SD_InitSPI();
	if ((SD_InitCard()) || (!FS_Init())) {
		SetTest1;
		ResetTest2;
		while (1);
	}
	if (!FS_FindBadAppleBin()) {
		ResetTest1;
		SetTest2;
		while (1);
	}
	SetTest1;
	SetTest2;
	
	while (BTN);
	ResetTest1;
	Player_Play();
	
	while (1) {
	}
}

