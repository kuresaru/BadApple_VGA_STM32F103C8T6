#include "stm32f10x.h"

#define SD_SetCS GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define SD_ResetCS GPIO_ResetBits(GPIOB, GPIO_Pin_12)

void SD_InitSPI(void);
u8 SD_InitCard(void); //成功返回0
u8 SD_StartReadBlock(u32 addr); //成功返回1
u8 SD_SendNOP(void); //返回读到的数据
