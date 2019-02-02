#include "stm32f10x.h"
//#include "delay.h"
#include "SD.h"
#include "fs.h"

u32 FS_PatternSector;						// 分区地址
u8 FS_SectorPerClust;					// 每簇扇区数
u16 	FS_PersistSector;						// 保留扇区数
u32 FS_FatUseSector;						// FAT表占用扇区数
u32 FS_FatSector;								// FAT1表起始扇区号
u32 FILE_BadApple_StartClust;		// badapple.bin文件起始簇号
u32 FILE_BadApple_Length; //文件长度

// 读取FAT32文件系统信息
u8 FS_Init() {
	u16 i;
	SD_ResetCS;
	// 读MBR 取分区地址
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
	// 读DBR 取每簇扇区数 保留扇区数 表占用扇区数 根目录簇号假设为2
	if (SD_StartReadBlock(FS_PatternSector)) {
		for (i = 0; i < 0x000D; i++)
			SD_SendNOP();
		FS_SectorPerClust = SD_SendNOP(); // 0x000D 每簇扇区数
		FS_PersistSector = SD_SendNOP(); // 0x0E-0x0F 保留扇区数
		FS_PersistSector |= SD_SendNOP() << 8;
		i += 3;
		while (i++ < 0x0024)
			SD_SendNOP();
		FS_FatUseSector = SD_SendNOP(); // 0x24-0x27 每个FAT表占用扇区数
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

// 取下一个簇(FAT中本簇的数据)  0x0FFFFF是结束
u32 FS_GetNextClust(u32 clust) {
	u32 sector = FS_FatSector + (clust / 128); // 一个簇4字节 一个扇区存128个簇
	u8 pos = clust % 128; // 扇区里的第几个字节
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
	u32 nextClust = 2; // 一般的是2  不是2的不能用 不管了
	u32 rootSector;
	u8 clustSector, fatItem, j;
	
	FS_FatSector = FS_PatternSector + FS_PersistSector; // FAT1表的扇区 = 分区DBR扇区 + 保留扇区数
	rootSector = FS_FatSector + FS_FatUseSector + FS_FatUseSector; // 根(2簇)扇区 = FAT1扇区 + FAT占用扇区x2
	FILE_BadApple_StartClust = 0;
	SD_ResetCS;
	while (nextClust != 0x0FFFFFFF) { // 遍历根目录所有簇
		for (clustSector = 0; clustSector < FS_SectorPerClust; clustSector++) { // 遍历根目录一个簇的所有扇区
			if (SD_StartReadBlock(rootSector + (((nextClust - 2) * FS_SectorPerClust) + clustSector))) {
				for (fatItem = 0; fatItem < 16; fatItem++) { // 遍历根目录一个簇的一个扇区的一项  一项32字节 共16*32=512字节=1扇区
					for (j = 0; j < 8; j++) // 8字节的文件名
						file.FileName[j] = SD_SendNOP();
					for (j = 0; j < 3; j++) // 3字节的扩展名
						file.FileType[j] = SD_SendNOP();
					file.FileAttribute = SD_SendNOP(); // 1字节的属性
					if (file.FileAttribute != 0x20 || file.FileName[0] == 0xE5) { // 不是归档文件或已被删除 直接跳过
						for (j = 0; j < 20; j++) // 后边的20字节丢弃
							SD_SendNOP();
						continue;
					}
					for (j = 0; j < 8; j++) // 1字节的保留 1字节的创建时间毫秒 2字节创建时间 2字节创建日期 2字节最后读取日期 = 8字节丢弃
						SD_SendNOP();
					file.FileClust = SD_SendNOP() << 16; // 起始簇高2字节
					file.FileClust |= SD_SendNOP() << 24;
					for (j = 0; j < 4; j++) // 2字节修改时间 2字节修改日期 = 4字节丢弃
						SD_SendNOP();
					file.FileClust |= SD_SendNOP(); // 起始簇低2字节
					file.FileClust |= SD_SendNOP() << 8;
					file.FileLength = SD_SendNOP(); // 4字节的文件长度
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
