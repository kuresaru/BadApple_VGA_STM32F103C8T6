#include "stm32f10x.h"

#define SD_SetCS GPIO_SetBits(GPIOB, GPIO_Pin_12)
#define SD_ResetCS GPIO_ResetBits(GPIOB, GPIO_Pin_12)

void SD_InitSPI(void);
u8 SD_InitCard(void); //�ɹ�����0
u8 SD_StartReadBlock(u32 addr); //�ɹ�����1
u8 SD_SendNOP(void); //���ض���������
