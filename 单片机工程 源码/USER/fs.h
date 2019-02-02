#include "stm32f10x.h"

u8 FS_Init(void);
u8 FS_FindBadAppleBin(void);
u32 FS_GetNextClust(u32 clust);

struct FS_FILE {
	u8 FileName[8];
	u8 FileType[3];
	u8 FileAttribute;
	u32 FileClust;
	u32 FileLength;
};
