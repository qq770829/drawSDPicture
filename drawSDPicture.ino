/*!
 * @file SD.ino
 * @brief 从SD卡上读取bmp/jpg/jpeg格式图片并显示在屏幕上，bmp支持颜色深度16位/24位/32位，jpg只支持JFIF格式用windows的画图打开然后保存一下,就是这个格式了。
 * @n 本示例支持的主板有ESP8266
 * @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [YeHangYu](hangyu.ye@dfrobot.com)
 * @version  V0.1
 * @date  2020-03-13
 * @url https://github.com/DFRobot/DFRobot_GDL
 */
#include <SD.h>
#include <SPI.h>
#include "DFRobot_GDL.h"
#include "picdecoder.h"

//自定义通信引脚
/*M0*/
#if defined ARDUINO_SAM_ZERO
#define TFT_DC  7
#define TFT_CS  5
#define TFT_RST 9
#define TFT_SD  6
/*ESP32 and ESP8266*/
#elif defined(ESP32) || defined(ESP8266)
#define TFT_DC  D3
#define TFT_CS  D4
#define TFT_RST D5
#define TFT_SD  D6
/*AVR系列主板*/
#else
#define TFT_DC  2
#define TFT_CS  3
#define TFT_RST 6
#define TFT_SD  5
#endif
/**
 * @brief Constructor  硬件SPI通信的构造函数
 * @param dc  SPI通信的命令/数据线引脚
 * @param cs  SPI通信的片选引脚
 * @param rst  屏的复位引脚
 */
DFRobot_ST7789_240x240_HW_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST);
//DFRobot_ST7789_240x320_HW_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST);
/*M0主板下DMA传输*/
//DFRobot_ST7789_240x240_DMA_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST);
//DFRobot_ST7789_240x320_DMA_SPI screen(/*dc=*/TFT_DC,/*cs=*/TFT_CS,/*rst=*/TFT_RST);

/*
 *可供用户选择的宏定义颜色
 *COLOR_RGB565_BLACK   COLOR_RGB565_NAVY    COLOR_RGB565_DGREEN   COLOR_RGB565_DCYAN 
 *COLOR_RGB565_MAROON  COLOR_RGB565_PURPLE  COLOR_RGB565_OLIVE    COLOR_RGB565_LGRAY
 *COLOR_RGB565_DGRAY   COLOR_RGB565_BLUE    COLOR_RGB565_GREEN    COLOR_RGB565_CYAN  
 *COLOR_RGB565_RED     COLOR_RGB565_MAGENTA COLOR_RGB565_YELLOW   COLOR_RGB565_ORANGE           
 *COLOR_RGB565_WHITE
 */
void setup()
{
  //初始化串口
  Serial.begin(115200);
  while (!Serial)
  {
    ; //等待串口连接
  }
  //初始化屏幕
  screen.begin();
  //初始化SD卡，直到初始化成功
  while(1)
  {
    if (SD.begin(/*sdcs=*/TFT_SD))
    {
      Serial.println("initialization done.");
      break;
    }
    Serial.println("initialization failed!");
  }
}


//存放读取的目录下图标的文件名
char str[32];

void loop()
{
  /*设置屏幕颜色*/
  screen.fillScreen(COLOR_RGB565_PURPLE);

  /**
   * @brief 显示sd卡上的jpg图片
   * @param filename 要打开的SD卡文件名及路径
   * @param sx 开始显示的x坐标
   * @param sy 开始显示的y坐标
   * @param ex 结束显示的x坐标
   * @param ey 结束显示的y坐标
   * @param screenDrawPixel 在屏幕上画像素点的函数名
   */
  drawSDPicture(/*filename=*/"picture/ColorImage/219x220.jpg",/*sx=*/0,/*sy=*/0,/*ex=*/240,/*ey=*/240,/*screenDrawPixel=*/screenDrawPixel);

  /*
  *SD.open的mode参数
  *FILE_READ: 打开文件进行读取，从文件的开头开始
  *FILE_WRITE: 打开文件进行读写，从文件末尾开始
  */
  //批量显示图标
  File myDir = SD.open(/*directory name=*/"picture/Icon/",/*mode=*/FILE_READ);
  if(myDir)
  {
    /*设置屏幕颜色*/
    screen.fillScreen(COLOR_RGB565_WHITE);
    //每隔32像素显示一个图标，直到铺满屏幕或显示完所有图标
    for(uint16_t y = 10; y<screen.height()-32; y+=60)
    {
      for(uint16_t x = 10; x<screen.width()-32; x+=60)
      {
        //读取目录中下一个文件信息
        File entry = myDir.openNextFile();
        if (! entry)
        {
          //没有可以显示的图片了，跳出循环
          goto quit;
        }
        //拼接文件名
        strcpy(str,"picture/Icon/");
        strcat(str,entry.name());
        //调用解码函数显示图片
        drawSDPicture(str, x, y, x+32, y+32, screenDrawPixel);
      }
    }
  }
  else Serial.println("dir open fail");
  
quit:
  myDir.close();
  delay(1000);
}

//供解码函数调用，该函数的功能是在屏幕上画一个像素点
void screenDrawPixel(int16_t x, int16_t y, uint16_t color)
{
  //开始画点
  screen.startWrite();
  //屏幕画点
  screen.writePixel(x, y, color);
  //退出画点
  screen.endWrite();
}
