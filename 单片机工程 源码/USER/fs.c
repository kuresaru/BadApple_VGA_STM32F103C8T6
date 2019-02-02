#include "stm32f10x.h"
//#include "delay.h"
#include "SD.h"
#include "fs.h"

u32 FS_PatternSector;						// ������ַ
u8 FS_SectorPerClust;					// ÿ��������
u16 	FS_PersistSector;						// ����������
u32 FS_FatUseSector;						// FAT��ռ��������
u32 FS_FatSector;								// FAT1����ʼ������
u32 FILE_BadApple_StartClust;		// badapple.bin�ļ���ʼ�غ�
u32 FILE_BadApple_Length; //�ļ�����

// ��ȡFAT32�ļ�ϵͳ��Ϣ
u8 FS_Init() {
	u16 i;
	SD_ResetCS;
	// ��MBR ȡ������ַ
	if (SD_StartReadBlock(0x00000000)) {
		for (i = 0; i < 0x01C6; i++)
			SD_SendNOP();
		FS_PatternSector = SD_SendNOP();
		FS_PatternSector |= SD_SendNOP() << 8;
		FS_PatternSector |= SD_SendNOP() << 16;
		FS_PatternSector |= SD_SendNOP() << 24;
		i += 4;
		while (i++ < 0x202)
			SD_SendNOP();
	} else {
		SD_SetCS;
		return 0;
	}
	// ��DBR ȡÿ�������� ���������� ��ռ�������� ��Ŀ¼�غż���Ϊ2
	if (SD_StartReadBlock(FS_PatternSector)) {
		for (i = 0; i < 0x000D; i++)
			SD_SendNOP();
		FS_SectorPerClust = SD_SendNOP(); // 0x000D ÿ��������
		FS_PersistSector = SD_SendNOP(); // 0x0E-0x0F ����������
		FS_PersistSector |= SD_SendNOP() << 8;
		i += 3;
		while (i++ < 0x0024)
			SD_SendNOP();
		FS_FatUseSector = SD_SendNOP(); // 0x24-0x27 ÿ��FAT��ռ��������
		FS_FatUseSector |= SD_SendNOP() << 8;
		FS_FatUseSector |= SD_SendNOP() << 16;
		FS_FatUseSector |= SD_SendNOP() << 24;
		i += 4;
		while (i++ < 0x0202)
			SD_SendNOP();
		SD_SetCS;
		return 1;
	} else {
		SD_SetCS;
		return 0;
	}
}

// ȡ��һ����(FAT�б��ص�����)  0x0FFFFF�ǽ���
u32 FS_GetNextClust(u32 clust) {
	u32 sector = FS_FatSector + (clust / 128); // һ����4�ֽ� һ��������128����
	u8 pos = clust % 128; // ������ĵڼ����ֽ�
	u8 i;
	u32 result = 0;
	if (SD_StartReadBlock(sector)) {
		for(i = 0; i < 128; i++) {
			if (i == pos) {
				result = SD_SendNOP();
				result |= SD_SendNOP() << 8;
				result |= SD_SendNOP() << 16;
				result |= SD_SendNOP() << 24;
			} else {
				SD_SendNOP();
				SD_SendNOP();
				SD_SendNOP();
				SD_SendNOP();
			}
		}
		SD_SendNOP();
		SD_SendNOP();
	}
	return result;
}

u8 FS_FindBadAppleBin() {
	struct FS_FILE file;
	u32 nextClust = 2; // һ�����2  ����2�Ĳ����� ������
	u32 rootSector;
	u8 clustSector, fatItem, j;
	
	FS_FatSector = FS_PatternSector + FS_PersistSector; // FAT1������� = ����DBR���� + ����������
	rootSector = FS_FatSector + FS_FatUseSector + FS_FatUseSector; // ��(2��)���� = FAT1���� + FATռ������x2
	FILE_BadApple_StartClust = 0;
	SD_ResetCS;
	while (nextClust != 0x0FFFFFFF) { // ������Ŀ¼���д�
		for (clustSector = 0; clustSector < FS_SectorPerClust; clustSector++) { // ������Ŀ¼һ���ص���������
			if (SD_StartReadBlock(rootSector + (((nextClust - 2) * FS_SectorPerClust) + clustSector))) {
				for (fatItem = 0; fatItem < 16; fatItem++) { // ������Ŀ¼һ���ص�һ��������һ��  һ��32�ֽ� ��16*32=512�ֽ�=1����
					for (j = 0; j < 8; j++) // 8�ֽڵ��ļ���
						file.FileName[j] = SD_SendNOP();
					for (j = 0; j < 3; j++) // 3�ֽڵ���չ��
						file.FileType[j] = SD_SendNOP();
					file.FileAttribute = SD_SendNOP(); // 1�ֽڵ�����
					if (file.FileAttribute != 0x20 || file.FileName[0] == 0xE5) { // ���ǹ鵵�ļ����ѱ�ɾ�� ֱ������
						for (j = 0; j < 20; j++) // ��ߵ�20�ֽڶ���
							SD_SendNOP();
						continue;
					}
					for (j = 0; j < 8; j++) // 1�ֽڵı��� 1�ֽڵĴ���ʱ����� 2�ֽڴ���ʱ�� 2�ֽڴ������� 2�ֽ�����ȡ���� = 8�ֽڶ���
						SD_SendNOP();
					file.FileClust = SD_SendNOP() << 16; // ��ʼ�ظ�2�ֽ�
					file.FileClust |= SD_SendNOP() << 24;
					for (j = 0; j < 4; j++) // 2�ֽ��޸�ʱ�� 2�ֽ��޸����� = 4�ֽڶ���
						SD_SendNOP();
					file.FileClust |= SD_SendNOP(); // ��ʼ�ص�2�ֽ�
					file.FileClust |= SD_SendNOP() << 8;
					file.FileLength = SD_SendNOP(); // 4�ֽڵ��ļ�����
					file.FileLength |= SD_SendNOP() << 8;
					file.FileLength |= SD_SendNOP() << 16;
					file.FileLength |= SD_SendNOP() << 24;
					if (file.FileName[0] == 'B' && file.FileName[1] == 'A' && file.FileName[2] == 'D' && file.FileName[3] == 'A' && 
									file.FileName[4] == 'P' && file.FileName[5] == 'P' && file.FileName[6] == 'L' && file.FileName[7] == 'E' && 
									file.FileType[0] == 'B' && file.FileType[1] == 'I' && file.FileType[2] == 'N') {
						FILE_BadApple_StartClust = file.FileClust;
						FILE_BadApple_Length = file.FileLength;
					}
				}
				SD_SendNOP();
				SD_SendNOP();
				if (FILE_BadApple_StartClust)
					break;
			}
		}
		if (FILE_BadApple_StartClust)
			break;
		nextClust = FS_GetNextClust(nextClust);
	}
	SD_SetCS;
	if (FILE_BadApple_StartClust) {
		return 1;
	} else {
		return 0;
	}
}
