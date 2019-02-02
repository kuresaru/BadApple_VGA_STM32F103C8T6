单片机: STM32F103C8T6
显示器: 800x600@56Hz
实际显示分辨率: 400x200

接线:
PA0 - 按钮
PA1 - VGA场同步(14)
PA2 - MP3(USART串口, 9600)
PA4 - LED1(红)
PA5 - LED2(绿)
PA7 - 经过10K微调电位器到VGA的RGB(1,2,3) 在播放时让VGA1脚对地为0.6V
PA8 - VGA行同步(13)
PB12 - SD卡-CS
PB13 - SD卡-SCK
PB14 - SD卡-MISO
PB15 - SD卡-MOSI

上电后只亮LED1表示SD卡错误
两个LED都亮时按按钮开始播放
SD需要HDD模式MBR分区表

视频 https://www.bilibili.com/video/av42456330/