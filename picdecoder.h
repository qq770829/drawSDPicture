//可以解码JPEG/JPG和BMP
//对JPEG格式,把不应该显示的列和行的颜色计算省去了.
//在显示大图片的时候,速度明显提高
//对BMP,优化了显示支持,对大图片的行和列也进行了取舍
//把不应该显示的行和列去掉
//对BMP,支持16bit 24bit 和32bit图片显示
//对JPG,基本只支持JFIF格式
//如果不是JFIF格式,用windows的画图打开然后保存一下,就是这个格式了!
#ifndef __PICDECODER_H__
#define __PICDECODER_H__
//BMP信息头
//重定义区
typedef long  LONG; 
typedef unsigned long   DWORD;
typedef unsigned char   BYTE;
typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;
typedef unsigned short  WORD;
typedef unsigned int  UINT;


typedef  struct 
{
    DWORD biSize ;       //说明BITMAPINFOHEADER结构所需要的字数。
    LONG  biWidth ;      //说明图象的宽度，以象素为单位 
    LONG  biHeight ;     //说明图象的高度，以象素为单位 
    WORD  biPlanes ;     //为目标设备说明位面数，其值将总是被设为1 
    WORD  biBitCount ;     //说明比特数/象素，其值为1、4、8、16、24、或32
    DWORD biCompression ;  //说明图象数据压缩的类型。其值可以是下述值之一：
  //BI_RGB：没有压缩；
  //BI_RLE8：每个象素8比特的RLE压缩编码，压缩格式由2字节组成(重复象素计数和颜色索引)；  
    //BI_RLE4：每个象素4比特的RLE压缩编码，压缩格式由2字节组成
    //BI_BITFIELDS：每个象素的比特由指定的掩码决定。
    DWORD biSizeImage ;//说明图象的大小，以字节为单位。当用BI_RGB格式时，可设置为0  
    LONG  biXPelsPerMeter ;//说明水平分辨率，用象素/米表示
    LONG  biYPelsPerMeter ;//说明垂直分辨率，用象素/米表示
    DWORD biClrUsed ;    //说明位图实际使用的彩色表中的颜色索引数
    DWORD biClrImportant ; //说明对图象显示有重要影响的颜色索引的数目，如果是0，表示都重要。 
}__attribute__ ((packed)) BITMAPINFOHEADER ;
//BMP头文件
typedef  struct 
{
    WORD  bfType ;     //文件标志.只对'BM',用来识别BMP位图类型
    DWORD bfSize ;     //文件大小,占四个字节
    WORD  bfReserved1 ;//保留
    WORD  bfReserved2 ;//保留
    DWORD bfOffBits ;  //从文件开始到位图数据(bitmap data)开始之间的的偏移量
}__attribute__ ((packed)) BITMAPFILEHEADER ;
//彩色表 

typedef  struct 
{
    BYTE rgbBlue ;    //指定蓝色强度
    BYTE rgbGreen ;   //指定绿色强度 
    BYTE rgbRed ;   //指定红色强度 
    BYTE rgbReserved ;//保留，设置为0 
}__attribute__ ((packed)) RGBQUAD ;
//位图信息头
typedef  struct 
{ 
  BITMAPFILEHEADER bmfHeader;
  BITMAPINFOHEADER bmiHeader;  
  //RGBQUAD bmiColors[256];  
}__attribute__ ((packed)) BITMAPINFO; 
typedef RGBQUAD * LPRGBQUAD;//彩色表  

//图象数据压缩的类型
#define BI_RGB       0L
#define BI_RLE8      1L
#define BI_RLE4      2L
#define BI_BITFIELDS 3L 

#define M_SOF0  0xc0
#define M_DHT   0xc4
#define M_EOI   0xd9
#define M_SOS   0xda
#define M_DQT   0xdb
#define M_DRI   0xdd
#define M_APP0  0xe0      


#define W1 2841 /* 2048*sqrt(2)*cos(1*pi/16) */
#define W2 2676 /* 2048*sqrt(2)*cos(2*pi/16) */
#define W3 2408 /* 2048*sqrt(2)*cos(3*pi/16) */
#define W5 1609 /* 2048*sqrt(2)*cos(5*pi/16) */
#define W6 1108 /* 2048*sqrt(2)*cos(6*pi/16) */
#define W7 565  /* 2048*sqrt(2)*cos(7*pi/16) */
                         
                        
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
     
//宏定义            
#define WIDTHBYTES(i)    ((i+31)/32*4)//??????????
#define PI 3.1415926535     
//函数返回值定义
#define FUNC_OK 0
#define FUNC_MEMORY_ERROR 1
#define FUNC_FILE_ERROR 2
#define FUNC_FORMAT_ERROR 3     
//图片类型定义
#define BMPTYPE 1
#define JPGTYPE 2


//////////////////////////////////////////////////
//BMP解码函数
bool bmpDecode( u8 *filename);
//////////////////////////////////////////////////


//////////////////////////////////////////////////
//JPEG 解码函数        
int jpgDecode(u8* filename);
int  InitTag(void);
void InitTable(void);              //初始化数据表
int  Decode(void);                 //解码
int  DecodeMCUBlock(void);
int  HufBlock(u8 dchufindex,u8 achufindex);//哈夫曼解码
int  DecodeElement(void);            //解码一个像素
void IQtIZzMCUComponent(short flag);       //反量化
void IQtIZzBlock(short  *s ,short * d,short flag);
void GetYUV(short flag);     //色彩转换的实现,得到色彩空间数据
void StoreBuffer(void);

BYTE ReadByte(void);             //从文件里面读取一个字节出来
void Initialize_Fast_IDCT(void); //初始化反离散傅立叶变换
void Fast_IDCT(int * block);   //快速反离散傅立叶变换
void idctrow(int * blk);
void idctcol(int * blk);  
//////////////////////////////////////////////////   


bool AI_LoadPicFile( u8 *filename,u16 sx,u16 sy,u16 ex,u16 ey);//智能显示图片    
u8 pictype(u8* filename); //判断图片类型
#endif
