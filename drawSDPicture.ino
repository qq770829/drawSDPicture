/*!
 * @file SD.ino
 * @brief 从SD卡上读取bmp图片并显示在屏幕上。（支持颜色深度16位/24位/32位）图片宽高需要在屏幕尺寸范围内。
 * @n 本示例支持的主板有UNO, ESP32, ESP8266, FireBeetle-M0
 * @copyright  Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @author [YeHangYu](hangyu.ye@dfrobot.com)
 * @version  V0.1
 * @date  2020-01-18
 * @url https://github.com/DFRobot/DFRobot_GDL
 */

#include <SD.h>
#include <SPI.h>
//可以解码JPEG/JPG和BMP
//对JPEG格式,把不应该显示的列和行的颜色计算省去了.
//在显示大图片的时候,速度明显提高
//对BMP,优化了显示支持,对大图片的行和列也进行了取舍
//把不应该显示的行和列去掉
//对BMP,支持16bit 24bit 和32bit图片显示
//对JPG,基本只支持JFIF格式
//如果不是JFIF格式,用windows的画图打开然后保存一下,就是这个格式了!
#include "DFRobot_GDL.h"
#include "picdecoder.h"
//自定义通信引脚
/*M0*/  
#if defined ARDUINO_SAM_ZERO
#define TFT_DC  7
#define TFT_CS  5
#define TFT_RST 6
#define TFT_SD  9
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
 
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //初始化屏幕
  screen.begin();
  while(1)
  {
    if (!SD.begin(TFT_SD)) {
        Serial.println("initialization failed!");
        continue;
    }
    Serial.println("initialization done."); 
    break;
  }
}

void loop() {
	screen.fillScreen(COLOR_RGB565_PURPLE);
	AI_LoadPicFile((u8*)"picture/test/219x220.jpg", 0, 0, 240, 240);//目录和名字不能太长
  
  //批量显示图标
  File myFile = SD.open("picture/24/",FILE_READ);//打开dir
  //是否打开成功
  if(!myFile) 
  {
    Serial.println("open fail");
  }
  
  char str[32] ;
  screen.fillScreen(COLOR_RGB565_WHITE);
  for(uint16_t y = 10; y<screen.height()-32; y+=60){
    for(uint16_t x = 10; x<screen.width()-32; x+=60){
       File entry = myFile.openNextFile();
       if (! entry) 
	   {
      // no more files
      goto quit;
      }
      strcpy(str,"picture/24/");
      strcat(str,entry.name());
      AI_LoadPicFile((u8*)str, x, y, x+32, y+32);
    } 
  }
  
  quit:
  myFile.close();
  delay(1000);
}





//全局变量声明,BMP和JPEG共用    
//图像信息
typedef struct
{
  u32 ImgWidth; //图像的实际宽度和高度
  u32 ImgHeight;

  u32 Div_Fac;  //缩放系数 (扩大了10000倍的)

  u32 S_Height; //设定的高度和宽度
  u32 S_Width;

  u32 S_XOFF;   //x轴和y轴的偏移量
  u32 S_YOFF;

  u32 staticx;  //当前显示到的ｘｙ坐标
  u32 staticy;
}PIC_POS;

PIC_POS PICINFO;//图像位置信息



int screenWidth = 0, screenHeight = 0;//窗口大小
////////////////////////////////////////////////////  
/////////////////////////////////////////////////////    
//在JPEG函数里面用到的变量
short     SampRate_Y_H, SampRate_Y_V;
short     SampRate_U_H, SampRate_U_V;
short     SampRate_V_H, SampRate_V_V;
short     H_YtoU, V_YtoU, H_YtoV, V_YtoV;
short     Y_in_MCU, U_in_MCU, V_in_MCU;
unsigned char* lp;//取代lpJpegBuf
short     qt_table[3][64];
short     comp_num;
u8      comp_index[3];
u8        YDcIndex, YAcIndex, UVDcIndex, UVAcIndex;
u8      HufTabIndex;
short* YQtTable, * UQtTable, * VQtTable;
short       code_pos_table[4][16], code_len_table[4][16];
unsigned short  code_value_table[4][256];
unsigned short  huf_max_value[4][16], huf_min_value[4][16];
short     BitPos, CurByte;//byte的第几位,当前byte
short     rrun, vvalue;
short     MCUBuffer[10 * 64];
short     QtZzMCUBuffer[10 * 64];
short     BlockBuffer[64];
short     ycoef, ucoef, vcoef;
bool      IntervalFlag;
short     interval = 0;
short     Y[4 * 64], U[4 * 64], V[4 * 64];//
DWORD       sizei, sizej;
short       restart;

long* iclp;
//反Z字形编码表
const int Zig_Zag[8][8] = { {0,1,5,6,14,15,27,28},
              {2,4,7,13,16,26,29,42},
              {3,8,12,17,25,30,41,43},
              {9,11,18,24,31,40,44,53},
              {10,19,23,32,39,45,52,54},
              {20,22,33,38,46,51,55,60},
              {21,34,37,47,50,56,59,61},
              {35,36,48,49,57,58,62,63}
};
const BYTE And[9] = { 0,1,3,7,0xf,0x1f,0x3f,0x7f,0xff };
//数据缓冲区
u8 jpg_buffer[1024];//数据缓存区
u8 SPI_FLASH_BUF[4096];
long* iclip = (long*)SPI_FLASH_BUF;//4k BYTES //跟FLash共用
////////////////////////////////////////////////////

File f_bfile;
File f_jpeg;

//初始化智能画点
void AI_Drow_Init(void)
{
  float temp, temp1;
  temp = (float)PICINFO.S_Width / PICINFO.ImgWidth;
  temp1 = (float)PICINFO.S_Height / PICINFO.ImgHeight;

  if (temp < temp1)temp1 = temp;//取较小的那个   
  if (temp1 > 1)temp1 = 1;
  //使图片处于所给区域的中间
  PICINFO.S_XOFF += (PICINFO.S_Width - temp1 * PICINFO.ImgWidth) / 2;
  PICINFO.S_YOFF += (PICINFO.S_Height - temp1 * PICINFO.ImgHeight) / 2;
  temp1 *= 10000;//扩大10000倍  
  PICINFO.Div_Fac = temp1;

  PICINFO.staticx = 5000;
  PICINFO.staticy = 5000;//放到一个不可能的值上面                
}
//判断这个像素是否可以显示
//(x,y) :像素原始坐标
//chg   :功能变量. 
//返回值:0,不需要显示.1,需要显示
inline u8 IsElementOk(u16 x, u16 y, u8 chg)
{
  if (x != PICINFO.staticx || y != PICINFO.staticy)
  {
    if (chg == 1)
    {
      PICINFO.staticx = x;
      PICINFO.staticy = y;
    }
    return 1;
  }
  else return 0;
}
//FileName:要显示的图片文件  BMP/JPG/JPEG
//(sx,sy) :开始显示的坐标点
//(ex,ey) :结束显示的坐标点
//图片会在开始和结束的坐标点范围内显示
bool AI_LoadPicFile(u8* filename, u16 sx, u16 sy, u16 ex, u16 ey)
{
  //窗口大小
  screenWidth = ex, screenHeight = ey;
  
  int funcret;//返回值 
  //得到显示方框大小  
  u8 fileTpe;
  int index;
  for (index = 0; index < 1024; index++)
    iclip[index] = 0;

  if (ey > sy)PICINFO.S_Height = ey - sy;
  else PICINFO.S_Height = sy - ey;
  if (ex > sx)PICINFO.S_Width = ex - sx;
  else PICINFO.S_Width = sx - ex;
  //显示区域无效
  if (PICINFO.S_Height == 0 || PICINFO.S_Width == 0)
  {
    PICINFO.S_Height = screenHeight;
    PICINFO.S_Width = screenWidth;
    return false;
  }
  //显示的开始坐标点
  PICINFO.S_YOFF = sy;
  PICINFO.S_XOFF = sx;

  fileTpe = pictype(filename);  //得到文件的类型
  if (fileTpe == BMPTYPE)//得到一个BMP图像
  {
    funcret = bmpDecode(filename); 
    return funcret;
  }
  else if (fileTpe == JPGTYPE)//得到JPG/JPEG图片
  {
    funcret = jpgDecode(filename); 
    return funcret;
  }
  else return false;  //非图片格式!!!

  

  if (funcret == FUNC_OK)return true;//解码成功
  else return false;   //解码失败 
}

//解码这个BMP文件                         
bool bmpDecode(u8* filename)
{
  File f_bmp;
  u16 count;
  u8 res;
  u8  rgb, color_byte;
  unsigned char R, G, B;
  u16 x, y ,color; 
  u16 uiTemp;    //x轴方向像素计数器 
  u16 countpix = 0;//记录像素    
  //x,y的实际坐标  
  u16 realx = 0;
  u16 realy = 0;
  u8  yok = 1;
  u8  bitype;//判断压缩类型
  BITMAPINFO* pbmp;//临时指针 

  f_bmp = SD.open((const char*)filename, FILE_READ);//打开文件 
  if(f_bmp ==NULL) 
  {
	  Serial.println("open fail");
	  return false;
  }
  f_bmp.read(jpg_buffer, 1024);//读取一次
  pbmp = (BITMAPINFO*)jpg_buffer;//得到BMP的头部信息   
  (u8*)jpg_buffer;
  count = pbmp->bmfHeader.bfOffBits;        //数据偏移,得到数据段的开始地址
  color_byte = pbmp->bmiHeader.biBitCount / 8;//彩色位 16/24/32  
  PICINFO.ImgHeight = pbmp->bmiHeader.biHeight;//得到图片高度
  PICINFO.ImgWidth = pbmp->bmiHeader.biWidth;  //得到图片宽度   
  //水平像素必须是4的倍数!!
  if ((PICINFO.ImgWidth * color_byte) % 4)
    uiTemp = ((PICINFO.ImgWidth * color_byte) / 4 + 1) * 4;
  else
    uiTemp = PICINFO.ImgWidth * color_byte;
  if (color_byte == 2)  //16位分为RGB555和RGB565
  {
    if (pbmp->bmiHeader.biCompression == 0) //RGB555没有压缩 为0
      bitype = 0;
    else
      bitype = 1;
  }
  AI_Drow_Init();//初始化智能画图      
  
  
  //开始解码BMP                                
  x = 0;
  y = PICINFO.ImgHeight;
  rgb = 0;
  realy = y * PICINFO.Div_Fac / 10000;
  while (1)
  {
    while (count < 1024)  //读取一簇512扇区 (SectorsPerClust 每簇扇区数)
    {
      if (color_byte == 3)   //24位颜色图
      {
        switch (rgb)
        {
        case 0:
          B=jpg_buffer[count];
          break;
        case 1:
          G=jpg_buffer[count];
          break;
        case 2:
          R=jpg_buffer[count];
          break;
        }
      }
      else if (color_byte == 2)  //16位
      {
        switch (rgb)
        {
        case 0: //gggbbbbb
          B = (jpg_buffer[count] & 0x1f )<<3;
          G = jpg_buffer[count] >> 5;
          break;
        case 1:
          if (bitype == 0) //RGB555颜色图为没有压缩 （0rrrrrgg gggbbbbb）
          {
            G |= (jpg_buffer[count] & 0x3) << 3;
            G *= 8;
            R = jpg_buffer[count] << 1;
          }
          else //RGB565  （rrrrrggg gggbbbbb）
          {
            G |= (jpg_buffer[count] & 0x7)<<3;
            G *= 4;
            R = jpg_buffer[count]; 
          }
          break;
        }
      }
      else if (color_byte == 4)//32位颜色图
      {
        switch (rgb)
        {
        case 0:
          B = jpg_buffer[count];
          break;
        case 1:
          G = jpg_buffer[count];
          break;
        case 2:
          R = jpg_buffer[count];
          break;
        case 3:break;
        }
      }
      //位图颜色得到  
      rgb++;
      count++;
      if (rgb == color_byte) //水平方向读取到1像素数数据后显示
      {
        if (x < PICINFO.ImgWidth)
        {
          realx = x * PICINFO.Div_Fac / 10000;//x轴实际值
          if (IsElementOk(realx, realy, 1) && yok)//符合条件
          {
			color = R >> 3;
            color = color << 6;
            color |= (G >> 2);
            color = color << 5;
            color |= (B >> 3);
			//开始画点
			screen.startWrite(); 
            screen.writePixel( realx + PICINFO.S_XOFF, realy + PICINFO.S_YOFF, color);
			//退出画点
			screen.endWrite();
          }
        }
        x++;//x轴增加一个像素 
        rgb = 0;
      }
      countpix++;//像素累加
      if (countpix >= uiTemp)//水平方向像素值到了.换行
      {
        y--;
        if (y <= 0) {
          return true;//BMP显示结束.  
        }
        realy = y * PICINFO.Div_Fac / 10000;//实际y值改变   
        if (IsElementOk(realx, realy, 0))yok = 1;//此处不改变PICINFO.staticx,y的值  
        else yok = 0;
        x = 0;
        countpix = 0;
        rgb = 0;
      }
    }
    f_bmp.read(jpg_buffer, 1024);//读出
    count = 0;
  }
  return true;               
}

//对指针地址进行改变!
//pc    :当前指针
//返回值:当前指针的减少量.在d_buffer里面自动进行了偏移
void jpg_seek(u8* pbase, u8** pnow)
{
  u32 pos;
  u16 offset;
  offset = *pnow - pbase;//当前的偏移量.
  if (offset > 1000)//将要结束了,做下一次读取
  {
    //pos = ftell(f_jpeg);        //得到当前位置
    pos = f_jpeg.position();
    f_jpeg.seek(pos -1024 + offset);//因为之前读了1024还没有用完，所以指向没有用到的位置开始读
    f_jpeg.read(pbase, 1024); //读取1024个字节.
    *pnow = pbase;//复位
  }
}

//初始化d_buffer的数据                               
int InitTag(void)
{
  bool finish = false;
  u8 id;
  short  llength;
  short  i, j, k;
  short  huftab1, huftab2;
  short  huftabindex;
  u8 hf_table_index;
  u8 qt_table_index;
  u8 comnum;//最长为256个字节

  unsigned char* lptemp;
  short  colorount;

  lp = jpg_buffer + 2;//跳过两个字节SOI(0xFF，0xD8 Start of Image) 
//  lp-=P_Cal(lp); 
  jpg_seek(jpg_buffer, &lp);
  while (!finish)
  {
    id = *(lp + 1);//取出低位字节(高位在前，低位在后)
    lp += 2;     //跳过取出的字节
    //lp-=P_Cal(lp);
    jpg_seek(jpg_buffer, &lp);
    switch (id)
    {
    case M_APP0: //JFIF APP0 segment marker (0xE0)
           //标志应用数据段的开始       
      llength = MAKEWORD(*(lp + 1), *lp);//得到应用数据段长度  
      lp += llength;
      //  lp-=P_Cal(lp);  
      jpg_seek(jpg_buffer, &lp);
      break;
    case M_DQT: //定义量化表标记(0xFF,0xDB) 
      llength = MAKEWORD(*(lp + 1), *lp);//(量化表长度)两个字节 
      qt_table_index = (*(lp + 2)) & 0x0f;//量化表信息bit 0..3: QT 号(0..3, 否则错误) 
                      //bit 4..7: QT 精度, 0 = 8 bit, 否则 16 bit
      lptemp = lp + 3;            //n 字节的 QT, n = 64*(精度+1) 
      //d_buffer里面至少有有512个字节的余度,这里最大用到128个字节
      if (llength < 80)         //精度为 8 bit 
      {
        for (i = 0; i < 64; i++)qt_table[qt_table_index][i] = (short)*(lptemp++);
      }
      else              //精度为 16 bit 
      {
        for (i = 0; i < 64; i++)qt_table[qt_table_index][i] = (short)*(lptemp++);
        qt_table_index = (*(lptemp++)) & 0x0f;
        for (i = 0; i < 64; i++)qt_table[qt_table_index][i] = (short)*(lptemp++);
      }
      lp += llength;  //跳过量化表 
      //lp-=P_Cal(lp);
      jpg_seek(jpg_buffer, &lp);
      break;
    case M_SOF0:                  //帧开始 (baseline JPEG 0xFF,0xC0)
      llength = MAKEWORD(*(lp + 1), *lp);   //长度 (高字节, 低字节), 8+components*3 
      PICINFO.ImgHeight = MAKEWORD(*(lp + 4), *(lp + 3));//图片高度 (高字节, 低字节), 如果不支持 DNL 就必须 >0 
      PICINFO.ImgWidth = MAKEWORD(*(lp + 6), *(lp + 5));  //图片宽度 (高字节, 低字节), 如果不支持 DNL 就必须 >0 
      comp_num = *(lp + 7);//components 数量(1 u8), 灰度图是 1, YCbCr/YIQ 彩色图是 3, CMYK 彩色图是 4                                                
      if ((comp_num != 1) && (comp_num != 3))return FUNC_FORMAT_ERROR;// 格式错误  
      if (comp_num == 3)            //YCbCr/YIQ 彩色图
      {
        comp_index[0] = *(lp + 8);      //component id (1 = Y, 2 = Cb, 3 = Cr, 4 = I, 5 = Q) 
        SampRate_Y_H = (*(lp + 9)) >> 4;  //水平采样系数 
        SampRate_Y_V = (*(lp + 9)) & 0x0f;//垂直采样系数 
        YQtTable = (short*)qt_table[*(lp + 10)];//通过量化表号取得量化表地址 

        comp_index[1] = *(lp + 11);        //component id 
        SampRate_U_H = (*(lp + 12)) >> 4;      //水平采样系数
        SampRate_U_V = (*(lp + 12)) & 0x0f;    //垂直采样系数 
        UQtTable = (short*)qt_table[*(lp + 13)];//通过量化表号取得量化表地址

        comp_index[2] = *(lp + 14);        //component id 
        SampRate_V_H = (*(lp + 15)) >> 4;      //水平采样系数
        SampRate_V_V = (*(lp + 15)) & 0x0f;    //垂直采样系数 
        VQtTable = (short*)qt_table[*(lp + 16)];//通过量化表号取得量化表地址
      }
      else                     //component id 
      {
        comp_index[0] = *(lp + 8);
        SampRate_Y_H = (*(lp + 9)) >> 4;
        SampRate_Y_V = (*(lp + 9)) & 0x0f;
        YQtTable = (short*)qt_table[*(lp + 10)];//灰度图的量化表都一样

        comp_index[1] = *(lp + 8);
        SampRate_U_H = 1;
        SampRate_U_V = 1;
        UQtTable = (short*)qt_table[*(lp + 10)];

        comp_index[2] = *(lp + 8);
        SampRate_V_H = 1;
        SampRate_V_V = 1;
        VQtTable = (short*)qt_table[*(lp + 10)];
      }
      lp += llength;
      jpg_seek(jpg_buffer, &lp);
      break;
    case M_DHT: //定义哈夫曼表(0xFF,0xC4)             
      llength = MAKEWORD(*(lp + 1), *lp);//长度 (高字节, 低字节)
      if (llength < 0xd0)       // Huffman Table信息 (1 u8) 
      {
        huftab1 = (short)(*(lp + 2)) >> 4;     //huftab1=0,1(HT 类型,0 = DC 1 = AC)
        huftab2 = (short)(*(lp + 2)) & 0x0f;   //huftab2=0,1(HT 号  ,0 = Y  1 = UV)
        huftabindex = huftab1 * 2 + huftab2;   //0 = YDC 1 = UVDC 2 = YAC 3 = UVAC 
        lptemp = lp + 3;//!!!  
        //在这里可能出现余度不够,多于512字节,则会导致出错!!!!
        for (i = 0; i < 16; i++)             //16 bytes: 长度是 1..16 代码的符号数
          code_len_table[huftabindex][i] = (short)(*(lptemp++));//码长为i的码字个数   
        j = 0;
        for (i = 0; i < 16; i++)       //得出HT的所有码字的对应值 
        {
          if (code_len_table[huftabindex][i] != 0)
          {
            k = 0;
            while (k < code_len_table[huftabindex][i])
            {
              code_value_table[huftabindex][k + j] = (short)(*(lptemp++));//最可能的出错地方
              k++;
            }
            j += k;
          }
        }
        i = 0;
        while (code_len_table[huftabindex][i] == 0)i++;
        for (j = 0; j < i; j++)
        {
          huf_min_value[huftabindex][j] = 0;
          huf_max_value[huftabindex][j] = 0;
        }
        huf_min_value[huftabindex][i] = 0;
        huf_max_value[huftabindex][i] = code_len_table[huftabindex][i] - 1;
        for (j = i + 1; j < 16; j++)
        {
          huf_min_value[huftabindex][j] = (huf_max_value[huftabindex][j - 1] + 1) << 1;
          huf_max_value[huftabindex][j] = huf_min_value[huftabindex][j] + code_len_table[huftabindex][j] - 1;
        }
        code_pos_table[huftabindex][0] = 0;
        for (j = 1; j < 16; j++)
          code_pos_table[huftabindex][j] = code_len_table[huftabindex][j - 1] + code_pos_table[huftabindex][j - 1];
        lp += llength;
        //lp-=P_Cal(lp); 
        jpg_seek(jpg_buffer, &lp);
      }//if   
      else
      {
        hf_table_index = *(lp + 2);
        lp += 2;
        //lp-=P_Cal(lp);
        jpg_seek(jpg_buffer, &lp);
        while (hf_table_index != 0xff)
        {
          huftab1 = (short)hf_table_index >> 4;     //huftab1=0,1
          huftab2 = (short)hf_table_index & 0x0f;   //huftab2=0,1
          huftabindex = huftab1 * 2 + huftab2;
          lptemp = lp + 1;
          colorount = 0;
          for (i = 0; i < 16; i++)
          {
            code_len_table[huftabindex][i] = (short)(*(lptemp++));
            colorount += code_len_table[huftabindex][i];
          }
          colorount += 17;
          j = 0;
          for (i = 0; i < 16; i++)
          {
            if (code_len_table[huftabindex][i] != 0)
            {
              k = 0;
              while (k < code_len_table[huftabindex][i])
              {
                code_value_table[huftabindex][k + j] = (short)(*(lptemp++));//最可能出错的地方,余度不够
                k++;
              }
              j += k;
            }
          }
          i = 0;
          while (code_len_table[huftabindex][i] == 0)i++;
          for (j = 0; j < i; j++)
          {
            huf_min_value[huftabindex][j] = 0;
            huf_max_value[huftabindex][j] = 0;
          }
          huf_min_value[huftabindex][i] = 0;
          huf_max_value[huftabindex][i] = code_len_table[huftabindex][i] - 1;
          for (j = i + 1; j < 16; j++)
          {
            huf_min_value[huftabindex][j] = (huf_max_value[huftabindex][j - 1] + 1) << 1;
            huf_max_value[huftabindex][j] = huf_min_value[huftabindex][j] + code_len_table[huftabindex][j] - 1;
          }
          code_pos_table[huftabindex][0] = 0;
          for (j = 1; j < 16; j++)
            code_pos_table[huftabindex][j] = code_len_table[huftabindex][j - 1] + code_pos_table[huftabindex][j - 1];
          lp += colorount;
          //  lp-=P_Cal(lp);   
          jpg_seek(jpg_buffer, &lp);
          hf_table_index = *lp;
        }  //while
      }  //else
      break;
    case M_DRI://定义差分编码累计复位的间隔
      llength = MAKEWORD(*(lp + 1), *lp);
      restart = MAKEWORD(*(lp + 3), *(lp + 2));
      lp += llength;
      //lp-=P_Cal(lp);
      jpg_seek(jpg_buffer, &lp);
      break;
    case M_SOS:  //扫描开始 12字节
      llength = MAKEWORD(*(lp + 1), *lp);
      comnum = *(lp + 2);
      if (comnum != comp_num)return FUNC_FORMAT_ERROR; //格式错误   
      lptemp = lp + 3;//这里也可能出现错误
      //这里也可能出错,但是几率比较小了
      for (i = 0; i < comp_num; i++)//每组件的信息 
      {
        if (*lptemp == comp_index[0])
        {
          YDcIndex = (*(lptemp + 1)) >> 4;   //Y 使用的 Huffman 表 
          YAcIndex = ((*(lptemp + 1)) & 0x0f) + 2;
        }
        else
        {
          UVDcIndex = (*(lptemp + 1)) >> 4;   //U,V
          UVAcIndex = ((*(lptemp + 1)) & 0x0f) + 2;
        }
        lptemp += 2;//comp_num<256,但是2*comp_num+3可能>=512
      }
      lp += llength;
      //lp-=P_Cal(lp);
      jpg_seek(jpg_buffer, &lp);
      finish = true;
      break;
    case M_EOI:return FUNC_FORMAT_ERROR;//图片结束 标记     
    default:
      if ((id & 0xf0) != 0xd0)
      {
        llength = MAKEWORD(*(lp + 1), *lp);
        lp += llength;
        //lp-=P_Cal(lp); 
        jpg_seek(jpg_buffer, &lp);
      }
      else lp += 2;
      break;
    }  //switch
  } //while
  return FUNC_OK;
}
//初始化量化表，全部清零
void InitTable(void)
{
  short i, j;
  sizei = sizej = 0;
  PICINFO.ImgWidth = PICINFO.ImgHeight = 0;
  rrun = vvalue = 0;
  BitPos = 0;
  CurByte = 0;
  IntervalFlag = false;
  restart = 0;
  for (i = 0; i < 3; i++) //量化表
    for (j = 0; j < 64; j++)
      qt_table[i][j] = 0;
  comp_num = 0;
  HufTabIndex = 0;
  for (i = 0; i < 3; i++)
    comp_index[i] = 0;
  for (i = 0; i < 4; i++)
    for (j = 0; j < 16; j++) {
      code_len_table[i][j] = 0;
      code_pos_table[i][j] = 0;
      huf_max_value[i][j] = 0;
      huf_min_value[i][j] = 0;
    }
  for (i = 0; i < 4; i++)
    for (j = 0; j < 256; j++)
      code_value_table[i][j] = 0;

  for (i = 0; i < 10 * 64; i++) {
    MCUBuffer[i] = 0;
    QtZzMCUBuffer[i] = 0;
  }
  for (i = 0; i < 64; i++) {
    Y[i] = 0;
    U[i] = 0;
    V[i] = 0;
    BlockBuffer[i] = 0;
  }
  ycoef = ucoef = vcoef = 0;
}
//调用顺序: Initialize_Fast_IDCT() :初始化 
//          DecodeMCUBlock()       Huffman Decode 
//          IQtIZzMCUComponent()   反量化、反DCT 
//          GetYUV()               Get Y U V 
//          StoreBuffer()          YUV to RGB 
int Decode()
{
  int funcret;
  Y_in_MCU = SampRate_Y_H * SampRate_Y_V;//YDU YDU YDU YDU
  U_in_MCU = SampRate_U_H * SampRate_U_V;//cRDU 
  V_in_MCU = SampRate_V_H * SampRate_V_V;//cBDU
  H_YtoU = SampRate_Y_H / SampRate_U_H;
  V_YtoU = SampRate_Y_V / SampRate_U_V;
  H_YtoV = SampRate_Y_H / SampRate_V_H;
  V_YtoV = SampRate_Y_V / SampRate_V_V;
  Initialize_Fast_IDCT();
  while ((funcret = DecodeMCUBlock()) == FUNC_OK) //After Call DecodeMCUBUBlock()
  {
    interval++;                 //The Digital has been Huffman Decoded and 
    if ((restart) && (interval % restart == 0))//be stored in MCUBuffer(YDU,YDU,YDU,YDU           
      IntervalFlag = true;          // UDU,VDU) Every DU := 8*8 
    else
      IntervalFlag = false;
    IQtIZzMCUComponent(0); //反量化 and IDCT The Data in QtZzMCUBuffer 
    IQtIZzMCUComponent(1);
    IQtIZzMCUComponent(2);
    GetYUV(0);         //得到Y cR cB 
    GetYUV(1);
    GetYUV(2);
    StoreBuffer();       //To RGB
    sizej += SampRate_Y_H * 8;
    if (sizej >= PICINFO.ImgWidth)
    {
      sizej = 0;
      sizei += SampRate_Y_V * 8;
    }
    if ((sizej == 0) && (sizei >= PICINFO.ImgHeight))break;
  }
  return funcret;
}
// 入口 QtZzMCUBuffer 出口 Y[] U[] V[] 
//得到YUV色彩空间   
void  GetYUV(short flag)
{
  short H, VV;
  short i, j, k, h;
  short* buf = NULL;
  short* pQtZzMCU = NULL;
  switch (flag)
  {
  case 0://亮度分量
    H = SampRate_Y_H;
    VV = SampRate_Y_V;
    buf = Y;
    pQtZzMCU = QtZzMCUBuffer;
    break;
  case 1://红色分量
    H = SampRate_U_H;
    VV = SampRate_U_V;
    buf = U;
    pQtZzMCU = QtZzMCUBuffer + Y_in_MCU * 64;
    break;
  case 2://蓝色分量
    H = SampRate_V_H;
    VV = SampRate_V_V;
    buf = V;
    pQtZzMCU = QtZzMCUBuffer + (Y_in_MCU + U_in_MCU) * 64;
    break;
  }
  for (i = 0; i < VV; i++)
    for (j = 0; j < H; j++)
      for (k = 0; k < 8; k++)
        for (h = 0; h < 8; h++)
          buf[(i * 8 + k) * SampRate_Y_H * 8 + j * 8 + h] = *pQtZzMCU++;
}

//将解出的字按RGB形式存储 lpbmp (BGR),(BGR) ......入口Y[] U[] V[] 出口lpPtr   
void StoreBuffer()
{
  short i = 0, j = 0;
  unsigned char R, G, B;
  int y, u, v, rr, gg, bb;
  u16 color;
  //x,y的实际坐标  
  u16 realx = sizej;
  u16 realy = 0;
  

    for (i = 0; i < SampRate_Y_V * 8; i++)
    {
      if ((sizei + i) < PICINFO.ImgHeight)// sizei表示行 sizej 表示列 
      {
        realy = PICINFO.Div_Fac * (sizei + i) / 10000;//实际Y坐标                      
        //在这里不改变PICINFO.staticx和PICINFO.staticy的值 ,如果在这里改变,则会造成每块的第一个点不显示!!!
        if (!IsElementOk(realx, realy, 0))continue;//列值是否满足条件? 寻找满足条件的列

        for (j = 0; j < SampRate_Y_H * 8; j++)
        {
          if ((sizej + j) < PICINFO.ImgWidth)
          {
            realx = PICINFO.Div_Fac * (sizej + j) / 10000;//实际X坐标    
            //在这里改变PICINFO.staticx和PICINFO.staticy的值
            if (!IsElementOk(realx, realy, 1))continue;//列值是否满足条件? 寻找满足条件的行

            y = Y[i * 8 * SampRate_Y_H + j];
            u = U[(i / V_YtoU) * 8 * SampRate_Y_H + j / H_YtoU];
            v = V[(i / V_YtoV) * 8 * SampRate_Y_H + j / H_YtoV];
            rr = ((y << 8) + 18 * u + 367 * v) >> 8;
            gg = ((y << 8) - 159 * u - 220 * v) >> 8;
            bb = ((y << 8) + 411 * u - 29 * v) >> 8;
            R = (unsigned char)rr;
            G = (unsigned char)gg;
            B = (unsigned char)bb;
            if (rr & 0xffffff00) if (rr > 255) R = 255; else if (rr < 0) R = 0;
            if (gg & 0xffffff00) if (gg > 255) G = 255; else if (gg < 0) G = 0;
            if (bb & 0xffffff00) if (bb > 255) B = 255; else if (bb < 0) B = 0;
            color = R >> 3;
            color = color << 6;
            color |= (G >> 2);
            color = color << 5;
            color |= (B >> 3);
			//开始画点
			screen.startWrite(); 
            screen.writePixel(realx + PICINFO.S_XOFF, realy + PICINFO.S_YOFF, color);
            //退出画点
			screen.endWrite();
		  }
          else break;
        }
      }
      else break;
    }
}
//Huffman Decode   MCU 出口 MCUBuffer  入口Blockbuffer[  ] 
int DecodeMCUBlock(void)
{
  short* lpMCUBuffer;
  short i, j;
  int funcret;
  if (IntervalFlag)//差值复位 
  {
    lp += 2;
    //  lp-=P_Cal(lp);  
    jpg_seek(jpg_buffer, &lp);
    ycoef = ucoef = vcoef = 0;
    BitPos = 0;
    CurByte = 0;
  }
  switch (comp_num)
  {
  case 3:  //comp_num 指图的类型（彩色图、灰度图） 
    lpMCUBuffer = MCUBuffer;
    for (i = 0; i < SampRate_Y_H * SampRate_Y_V; i++)  //Y
    {
      funcret = HufBlock(YDcIndex, YAcIndex);//解码4 * (8*8) 
      if (funcret != FUNC_OK)
        return funcret;
      BlockBuffer[0] = BlockBuffer[0] + ycoef;//直流分量是差值，所以要累加。
      ycoef = BlockBuffer[0];
      for (j = 0; j < 64; j++)
        *lpMCUBuffer++ = BlockBuffer[j];
    }
    for (i = 0; i < SampRate_U_H * SampRate_U_V; i++)  //U
    {
      funcret = HufBlock(UVDcIndex, UVAcIndex);
      if (funcret != FUNC_OK)
        return funcret;
      BlockBuffer[0] = BlockBuffer[0] + ucoef;
      ucoef = BlockBuffer[0];
      for (j = 0; j < 64; j++)
        *lpMCUBuffer++ = BlockBuffer[j];
    }
    for (i = 0; i < SampRate_V_H * SampRate_V_V; i++)  //V
    {
      funcret = HufBlock(UVDcIndex, UVAcIndex);
      if (funcret != FUNC_OK)
        return funcret;
      BlockBuffer[0] = BlockBuffer[0] + vcoef;
      vcoef = BlockBuffer[0];
      for (j = 0; j < 64; j++)
        *lpMCUBuffer++ = BlockBuffer[j];
    }
    break;
  case 1: //灰度图
    lpMCUBuffer = MCUBuffer;
    funcret = HufBlock(YDcIndex, YAcIndex);
    if (funcret != FUNC_OK)
      return funcret;
    BlockBuffer[0] = BlockBuffer[0] + ycoef;
    ycoef = BlockBuffer[0];
    for (j = 0; j < 64; j++)
      *lpMCUBuffer++ = BlockBuffer[j];
    for (i = 0; i < 128; i++)
      *lpMCUBuffer++ = 0;
    break;
  default:
    return FUNC_FORMAT_ERROR;
  }
  return FUNC_OK;
}
//Huffman Decode （8*8） DU   出口 Blockbuffer[ ] 入口 vvalue 
int HufBlock(u8 dchufindex, u8 achufindex)
{
  short count = 0;
  short i;
  int funcret;
  //dc
  HufTabIndex = dchufindex;
  funcret = DecodeElement();
  if (funcret != FUNC_OK)return funcret;
  BlockBuffer[count++] = vvalue;//解出的直流系数 
  //ac
  HufTabIndex = achufindex;
  while (count < 64)
  {
    funcret = DecodeElement();
    if (funcret != FUNC_OK)
      return funcret;
    if ((rrun == 0) && (vvalue == 0))
    {
      for (i = count; i < 64; i++)BlockBuffer[i] = 0;
      count = 64;
    }
    else
    {
      for (i = 0; i < rrun; i++)BlockBuffer[count++] = 0;//前面的零  
      BlockBuffer[count++] = vvalue;//解出的值 
    }
  }
  return FUNC_OK;
}
//Huffman 解码  每个元素   出口 vvalue 入口 读文件ReadByte 
int DecodeElement()
{
  int thiscode, tempcode;
  unsigned short temp, valueex;
  short codelen;
  u8 hufexbyte, runsize, tempsize, sign;
  u8 newbyte, lastbyte;

  if (BitPos >= 1) //BitPos指示当前比特位置 
  {
    BitPos--;
    thiscode = (u8)CurByte >> BitPos;//取一个比特
    CurByte = CurByte & And[BitPos];   //清除取走的比特位 
  }
  else                 //取出的一个字节已用完
  {                  //新取
    lastbyte = ReadByte();       //读出一个字节
    BitPos--;            //and[]:=0x0,0x1,0x3,0x7,0xf,0x1f,0x2f,0x3f,0x4f 
    newbyte = CurByte & And[BitPos];
    thiscode = lastbyte >> 7;
    CurByte = newbyte;
  }
  codelen = 1;
  //与Huffman表中的码字匹配，直自找到为止 
  while ((thiscode < huf_min_value[HufTabIndex][codelen - 1]) ||
    (code_len_table[HufTabIndex][codelen - 1] == 0) ||
    (thiscode > huf_max_value[HufTabIndex][codelen - 1]))
  {
    if (BitPos >= 1)//取出的一个字节还有
    {
      BitPos--;
      tempcode = (u8)CurByte >> BitPos;
      CurByte = CurByte & And[BitPos];
    }
    else
    {
      lastbyte = ReadByte();
      BitPos--;
      newbyte = CurByte & And[BitPos];
      tempcode = (u8)lastbyte >> 7;
      CurByte = newbyte;
    }
    thiscode = (thiscode << 1) + tempcode;
    codelen++;
    if (codelen > 16)return FUNC_FORMAT_ERROR;
  }  //while
  temp = thiscode - huf_min_value[HufTabIndex][codelen - 1] + code_pos_table[HufTabIndex][codelen - 1];
  hufexbyte = (u8)code_value_table[HufTabIndex][temp];
  rrun = (short)(hufexbyte >> 4);  //一个字节中，高四位是其前面的零的个数。
  runsize = hufexbyte & 0x0f;    //后四位为后面字的尺寸
  if (runsize == 0)
  {
    vvalue = 0;
    return FUNC_OK;
  }
  tempsize = runsize;
  if (BitPos >= runsize)
  {
    BitPos -= runsize;
    valueex = (u8)CurByte >> BitPos;
    CurByte = CurByte & And[BitPos];
  }
  else
  {
    valueex = CurByte;
    tempsize -= BitPos;
    while (tempsize > 8)
    {
      lastbyte = ReadByte();
      valueex = (valueex << 8) + (u8)lastbyte;
      tempsize -= 8;
    }  //while
    lastbyte = ReadByte();
    BitPos -= tempsize;
    valueex = (valueex << tempsize) + (lastbyte >> BitPos);
    CurByte = lastbyte & And[BitPos];
  }  //else
  sign = valueex >> (runsize - 1);
  if (sign)vvalue = valueex;//解出的码值 
  else
  {
    valueex = valueex ^ 0xffff;
    temp = 0xffff << runsize;
    vvalue = -(short)(valueex ^ temp);
  }
  return FUNC_OK;
}
//反量化MCU中的每个组件   入口 MCUBuffer 出口 QtZzMCUBuffer 
void IQtIZzMCUComponent(short flag)
{
  short H, VV;
  short i, j;
  short* pQtZzMCUBuffer = NULL;
  short* pMCUBuffer = NULL;

  switch (flag) {
  case 0:
    H = SampRate_Y_H;
    VV = SampRate_Y_V;
    pMCUBuffer = MCUBuffer;
    pQtZzMCUBuffer = QtZzMCUBuffer;
    break;
  case 1:
    H = SampRate_U_H;
    VV = SampRate_U_V;
    pMCUBuffer = MCUBuffer + Y_in_MCU * 64;
    pQtZzMCUBuffer = QtZzMCUBuffer + Y_in_MCU * 64;
    break;
  case 2:
    H = SampRate_V_H;
    VV = SampRate_V_V;
    pMCUBuffer = MCUBuffer + (Y_in_MCU + U_in_MCU) * 64;
    pQtZzMCUBuffer = QtZzMCUBuffer + (Y_in_MCU + U_in_MCU) * 64;
    break;
  }
  for (i = 0; i < VV; i++)
    for (j = 0; j < H; j++)
      IQtIZzBlock(pMCUBuffer + (i * H + j) * 64, pQtZzMCUBuffer + (i * H + j) * 64, flag);
}
//要量化的字 
//反量化 8*8 DU 
void IQtIZzBlock(short* s, short* d, short flag)
{
  short i, j;
  short tag;
  short* pQt = NULL;
  int buffer2[8][8];
  int* buffer1;
  short offset;

  switch (flag)
  {
  case 0:   //亮度
    pQt = YQtTable;
    offset = 128;
    break;
  case 1:   //红
    pQt = UQtTable;
    offset = 0;
    break;
  case 2:   //蓝 
    pQt = VQtTable;
    offset = 0;
    break;
  }

  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
    {
      tag = Zig_Zag[i][j];
      buffer2[i][j] = (int)s[tag] * (int)pQt[tag];
    }
  buffer1 = (int*)buffer2;
  Fast_IDCT(buffer1);//反DCT
  for (i = 0; i < 8; i++)
    for (j = 0; j < 8; j++)
      d[i * 8 + j] = buffer2[i][j] + offset;
}
//快速反DCT
void Fast_IDCT(int* block)
{
  short i;
  for (i = 0; i < 8; i++)idctrow(block + 8 * i);
  for (i = 0; i < 8; i++)idctcol(block + i);
}
//从源文件读取一个字节
u8 ReadByte(void)
{
  u8 i;
  i = *lp++;
  //lp-=P_Cal(lp);//经过P_Cal的处理,把指针移动
  jpg_seek(jpg_buffer, &lp);
  if (i == 0xff)lp++;
  BitPos = 8;
  CurByte = i;
  return i;
}
//初始化快速反DCT
void Initialize_Fast_IDCT(void)
{
  short i;

  iclp = iclip + 512;
  for (i = -512; i < 512; i++)
    iclp[i] = (i < -256) ? -256 : ((i > 255) ? 255 : i);
}
////////////////////////////////////////////////////////////////////////
void idctrow(int* blk)
{
  int x0, x1, x2, x3, x4, x5, x6, x7, x8;
  //intcut
  if (!((x1 = blk[4] << 11) | (x2 = blk[6]) | (x3 = blk[2]) |
    (x4 = blk[1]) | (x5 = blk[7]) | (x6 = blk[5]) | (x7 = blk[3])))
  {
    blk[0] = blk[1] = blk[2] = blk[3] = blk[4] = blk[5] = blk[6] = blk[7] = blk[0] << 3;
    return;
  }
  x0 = (blk[0] << 11) + 128; // for proper rounding in the fourth stage 
  //first stage
  x8 = W7 * (x4 + x5);
  x4 = x8 + (W1 - W7) * x4;
  x5 = x8 - (W1 + W7) * x5;
  x8 = W3 * (x6 + x7);
  x6 = x8 - (W3 - W5) * x6;
  x7 = x8 - (W3 + W5) * x7;
  //second stage
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6 * (x3 + x2);
  x2 = x1 - (W2 + W6) * x2;
  x3 = x1 + (W2 - W6) * x3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;
  //third stage
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181 * (x4 + x5) + 128) >> 8;
  x4 = (181 * (x4 - x5) + 128) >> 8;
  //fourth stage
  blk[0] = (x7 + x1) >> 8;
  blk[1] = (x3 + x2) >> 8;
  blk[2] = (x0 + x4) >> 8;
  blk[3] = (x8 + x6) >> 8;
  blk[4] = (x8 - x6) >> 8;
  blk[5] = (x0 - x4) >> 8;
  blk[6] = (x3 - x2) >> 8;
  blk[7] = (x7 - x1) >> 8;
}
//////////////////////////////////////////////////////////////////////////////
void idctcol(int* blk)
{
  int x0, x1, x2, x3, x4, x5, x6, x7, x8;
  //intcut
  if (!((x1 = (blk[8 * 4] << 8)) | (x2 = blk[8 * 6]) | (x3 = blk[8 * 2]) |
    (x4 = blk[8 * 1]) | (x5 = blk[8 * 7]) | (x6 = blk[8 * 5]) | (x7 = blk[8 * 3])))
  {
    blk[8 * 0] = blk[8 * 1] = blk[8 * 2] = blk[8 * 3] = blk[8 * 4] = blk[8 * 5]
      = blk[8 * 6] = blk[8 * 7] = iclp[(blk[8 * 0] + 32) >> 6];
    return;
  }
  x0 = (blk[8 * 0] << 8) + 8192;
  //first stage
  x8 = W7 * (x4 + x5) + 4;
  x4 = (x8 + (W1 - W7) * x4) >> 3;
  x5 = (x8 - (W1 + W7) * x5) >> 3;
  x8 = W3 * (x6 + x7) + 4;
  x6 = (x8 - (W3 - W5) * x6) >> 3;
  x7 = (x8 - (W3 + W5) * x7) >> 3;
  //second stage
  x8 = x0 + x1;
  x0 -= x1;
  x1 = W6 * (x3 + x2) + 4;
  x2 = (x1 - (W2 + W6) * x2) >> 3;
  x3 = (x1 + (W2 - W6) * x3) >> 3;
  x1 = x4 + x6;
  x4 -= x6;
  x6 = x5 + x7;
  x5 -= x7;
  //third stage
  x7 = x8 + x3;
  x8 -= x3;
  x3 = x0 + x2;
  x0 -= x2;
  x2 = (181 * (x4 + x5) + 128) >> 8;
  x4 = (181 * (x4 - x5) + 128) >> 8;
  //fourth stage
  blk[8 * 0] = iclp[(x7 + x1) >> 14];
  blk[8 * 1] = iclp[(x3 + x2) >> 14];
  blk[8 * 2] = iclp[(x0 + x4) >> 14];
  blk[8 * 3] = iclp[(x8 + x6) >> 14];
  blk[8 * 4] = iclp[(x8 - x6) >> 14];
  blk[8 * 5] = iclp[(x0 - x4) >> 14];
  blk[8 * 6] = iclp[(x3 - x2) >> 14];
  blk[8 * 7] = iclp[(x7 - x1) >> 14];
}
//判断图片格式
u8 pictype(u8* filename)
{
	if (filename[strlen((const char*)filename) - 1] == 'p'||filename[strlen((const char*)filename) - 1] == 'P') return 1;
	else if(filename[strlen((const char*)filename) - 1] == 'g'||filename[strlen((const char*)filename) - 1] == 'G') return 2;
	return 0;
}

//jpg图片解码显示
int jpgDecode(u8* filename)
{
	f_jpeg = SD.open((const char*)filename, FILE_READ);//打开文件
	if (f_jpeg == NULL)
	{
	Serial.println("open fail");
	return 0;
	}
	int funcret;
	f_jpeg.read(jpg_buffer,1024);
	InitTable();
	if ((funcret = InitTag()) != FUNC_OK)return false;
	if ((SampRate_Y_H == 0) || (SampRate_Y_V == 0))return false;
	AI_Drow_Init();
	funcret = Decode();//解码并逐行画点
	f_jpeg.close();
	return funcret;
}
