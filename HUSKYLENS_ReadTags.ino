/***************************************************
  Sugar Caculator

  2022.09.24


 ****************************************************/


/* Setting for HUSKYLENS */
#include "HUSKYLENS.h"
#include "SoftwareSerial.h"
HUSKYLENS huskylens;
SoftwareSerial mySerial(10, 11);                            /* RX, TX  HUSKYLENS green line >> Pin 10; blue line >> Pin 11 */

/* Setting for Screen */
#include "oledfont.h"
#include "Wire.h"
#define res A3      //RES
#define OLED_RES_Clr() digitalWrite(res,LOW)   //RES
#define OLED_RES_Set() digitalWrite(res,HIGH)  // SDA - A4, SCL - A5

#define OLED_CMD  0  //写命令
#define OLED_DATA 1 //写数据

/* Setting for TouchPad */
#define touchPin 12

/* Setting for LEDs */
#define Led_R 7
#define Led_G 5
#define Led_Y 4

/* Setting for Scale_HX711 */
#include <Arduino.h>
#include "HX711.h"
const int LOADCELL_DOUT_PIN = 2;   //DT blue- 2
const int LOADCELL_SCK_PIN = 3;   // SCK purple - 3
HX711 scale;
float currentWeight = 0;

/* Globle Flags */
bool tagScaned = false;
bool foodOn = false;

/* Globle Variables */
float currentSugarValStand;
float currentFoodSugarVal;
float sumSugarVal = 0;

/* Setting for Database */
typedef struct {
  uint8_t id;
  char* foodName;
  uint16_t sugarValue;
} foodTable;

const foodTable myFoodTableArr[] {       /* id; name; g */
  {0, "lemon", 5},    // lemon, avocado, 
  {1, "bayberry", 6},  // bayberry,melon,stawberry,pawpaw,star fruit
  {2, "watermelon", 7}, //watermelon,mango,Hami fruit
  {3, "Plum", 8},  // Plum,apricotlute
  {4, "grapefruit", 9},  //grapefruit,peach,grape,pineapple,
  {5, "orange", 10}, // orange,mulberry,cherry,pear
  {6, "pitaya", 12},  //pytaya,kiwi,apple
  {7, "litchi", 16}, //litchi,longan,
  {8, "banana", 22}, //banana,hawthron
  {9, "durian", 27}, //durian,date
  {10, "FRENCH", 222},
  {11, "Apple", 333},
  {12, "SPANISH", 444},
  {13, "FRENCH", 555},
  {14, "Apple", 666},
  {15, "SPANISH", 777},
  {16, "FRENCH", 888}
};

/* Setting for RTC */
#include <DS1307RTC.h>
#include <Wire.h>
tmElements_t tm;

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

/* Setting for Voice Module */
#include <DS1307RTC.h>

#include "XFS.h"          /*封装好的命令库*/
#include "TextTab.h"      /*中文需要放在该记事本中（因为编码不兼容）*/

#include <Wire.h>

XFS5152CE xfs;          /*实例化语音合成对象*/


/*超时设置，示例为30S*/
static uint32_t LastSpeakTime = 0;
#define SpeakTimeOut 10000

uint8_t n = 1;
static void XFS_Init()
{

  xfs.Begin(0x30);//设备i2c地址，地址为0x50
  delay(n);
  xfs.SetReader(XFS5152CE::Reader_XiaoYan);        //设置发音人
  delay(n);
  xfs.SetEncodingFormat(XFS5152CE::GB2312);           //文本的编码格式
  delay(n);
  xfs.SetLanguage(xfs.Language_Auto);                 //语种判断
  delay(n);
  xfs.SetStyle(XFS5152CE::Style_Continue);            //合成风格设置
  delay(n);
  xfs.SetArticulation(XFS5152CE::Articulation_Letter);  //设置单词的发音方式
  delay(n);
  xfs.SetSpeed(5);                         //设置语速1~10
  delay(n);
  xfs.SetIntonation(5);                    //设置语调1~10
  delay(n);
  xfs.SetVolume(10);                        //设置音量1~10
  delay(n);
}

unsigned char result = 0xFF;


/* Setting for HUSKYLEN Result */
void printResult(HUSKYLENSResult result);

/*===============================================================================*/

void setup() {
  pinMode(13, OUTPUT);
  pinMode(Led_R, OUTPUT);
  pinMode(Led_G, OUTPUT);
  pinMode(Led_Y, OUTPUT);

  pinMode(touchPin, INPUT);

  /* Setting for HUSKYLENS */
  Serial.begin(115200);
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  mySerial.begin(9600);

  /*Init the Scale */
  scale.set_scale(391.6);
  scale.tare();

  /* make sure huskeylens is connected  */
  while (!huskylens.begin(mySerial))
  {
    Serial.println(F("Begin failed!"));
    Serial.println(F("1.Please recheck the \"Protocol Type\" in HUSKYLENS (General Settings>>Protocol Type>>Serial 9600)"));
    Serial.println(F("2.Please recheck the connection."));
    delay(100);
  }

  /*Init the Voice */
  XFS_Init();
  xfs.StartSynthesis(TextTab1[0]);
  while (xfs.GetChipStatus() != xfs.ChipStatus_Idle)
  {
    delay(30);
  }

  /* Init for OLED Screen */
  OLED_Init();
  OLED_ColorTurn(0);//0正常显示 1反色显示
  OLED_DisplayTurn(0);//0正常显示 1翻转180度显示

  /* Init for RTC */
  initRTC();

}

/*===============================================================================*/

void loop() {

  if (!huskylens.request()) Serial.println(F("Fail to request data from HUSKYLENS, recheck the connection!"));
  else if (!huskylens.isLearned()) Serial.println(F("Nothing learned, press learn button on HUSKYLENS to learn one!"));
  else if (!huskylens.available() && tagScaned == false) {

    /* nothing in the camera, to the following things */
    Serial.println(F("No Tags Found."));
  }
  else if (tagScaned == false) {

    HUSKYLENSResult result = huskylens.read();
    printResult(result);

    /* search scanned object in the struct */

    for (uint8_t i = 0; i < sizeof(myFoodTableArr) / sizeof(foodTable); ++i) {
      //Serial.println(myFoodTableArr[i].sugarValue);

      if (result.ID == myFoodTableArr[i].id) {

        digitalWrite(Led_R, HIGH);                                                        /* light up the red led */
        tagScaned = true;
        OLED_ShowNum(20, 3, myFoodTableArr[i].id, 2, 8);                                  /* id tag */
        OLED_ShowNum(100, 3, myFoodTableArr[i].sugarValue, 2 , 8);                        /* id value */
        OLED_ShowString(60, 5,  myFoodTableArr[i].foodName, 8);                           /* id name */

        currentSugarValStand = myFoodTableArr[i].sugarValue;                              /* update the globle sugar value */
      }
    }

  }

  /* get the object's weight here, then do the caculation */
  if (tagScaned) {
    currentWeight = scale.get_units(10);                                                   /* get the food's weight in "g" */
    currentFoodSugarVal =  (currentWeight / 100 ) *  currentSugarValStand;                 /* need to caculate*/
    Serial.println(currentFoodSugarVal);
  }

  /* an indicator to make sure object is on the scale  */
  if (currentWeight = scale.get_units(10) > 10) {
    digitalWrite(Led_G, HIGH);                                                              /* led Green keep on is the right state for object */
    foodOn = true;

    sayWord(3);
    
  } else {
    digitalWrite(Led_G, LOW);
    foodOn = false;
  }


  if (tagScaned == true && foodOn == true) {
    if (digitalRead(touchPin) == HIGH) {

      sumSugarVal += currentFoodSugarVal;
      OLED_ShowNum(50, 8, sumSugarVal, 5 , 8);                                              /* update the sum of sugar */
      OLED_Clear();                                                                         /* clear the OLED */
      tagScaned = false;                                                                    /* reverse the Flags */
      digitalWrite(Led_R, LOW);                                                             /* turn off the Red LED */
    }
  }

  ShowOLEDInfo();                                                                            /* keep showing the title on the OLED */


  if (RTC.read(tm)) {
    Serial.print("Ok, Time = ");
    print2digits(tm.Hour);
    Serial.write(':');
    print2digits(tm.Minute);
    Serial.write(':');
    print2digits(tm.Second);
    Serial.println();
  } else {
    if (RTC.chipPresent()) {
      Serial.println("The DS1307 is stopped.  Please run the SetTime");
      Serial.println("example to initialize the time and begin running.");
      Serial.println();
    } else {
      Serial.println("DS1307 read error!  Please check the circuitry.");
      Serial.println();
    }
    delay(9000);
  }

}

void printResult(HUSKYLENSResult result) {
  Serial.println(String() + result.ID);
}

void OLED_ColorTurn(u8 i)
{
  if (!i) OLED_WR_Byte(0xA6, OLED_CMD); //正常显示
  else  OLED_WR_Byte(0xA7, OLED_CMD); //反色显示
}

//屏幕旋转180度
void OLED_DisplayTurn(u8 i)
{
  if (i == 0)
  {
    OLED_WR_Byte(0xC8, OLED_CMD); //正常显示
    OLED_WR_Byte(0xA1, OLED_CMD);
  }
  else
  {
    OLED_WR_Byte(0xC0, OLED_CMD); //反转显示
    OLED_WR_Byte(0xA0, OLED_CMD);
  }
}

//发送一个字节
//向SSD1306写入一个字节。
//mode:数据/命令标志 0,表示命令;1,表示数据;
void OLED_WR_Byte(u8 dat, u8 mode)
{
  Wire.beginTransmission(0x3c);
  if (mode) {
    Wire.write(0x40);
  }
  else {
    Wire.write(0x00);
  }
  Wire.write(dat); // sends one byte
  Wire.endTransmission();    // stop transmitting
}

//坐标设置
void OLED_Set_Pos(u8 x, u8 y)
{
  OLED_WR_Byte(0xb0 + y, OLED_CMD);
  OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
  OLED_WR_Byte((x & 0x0f), OLED_CMD);
}

//开启OLED显示
void OLED_Display_On(void)
{
  OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC命令
  OLED_WR_Byte(0X14, OLED_CMD); //DCDC ON
  OLED_WR_Byte(0XAF, OLED_CMD); //DISPLAY ON
}

//关闭OLED显示
void OLED_Display_Off(void)
{
  OLED_WR_Byte(0X8D, OLED_CMD); //SET DCDC命令
  OLED_WR_Byte(0X10, OLED_CMD); //DCDC OFF
  OLED_WR_Byte(0XAE, OLED_CMD); //DISPLAY OFF
}

//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!
void OLED_Clear(void)
{
  u8 i, n;
  for (i = 0; i < 8; i++)
  {
    OLED_WR_Byte (0xb0 + i, OLED_CMD); //设置页地址（0~7）
    OLED_WR_Byte (0x00, OLED_CMD);     //设置显示位置—列低地址
    OLED_WR_Byte (0x10, OLED_CMD);     //设置显示位置—列高地址
    for (n = 0; n < 128; n++)OLED_WR_Byte(0, OLED_DATA);
  } //更新显示
}


//在指定位置显示一个字符
//x:0~127
//y:0~63
//sizey:选择字体 6x8  8x16
void OLED_ShowChar(u8 x, u8 y, const u8 chr, u8 sizey)
{
  u8 c = 0, sizex = sizey / 2, temp;
  u16 i = 0, size1;
  if (sizey == 8)size1 = 6;
  else size1 = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * (sizey / 2);
  c = chr - ' '; //得到偏移后的值
  OLED_Set_Pos(x, y);
  for (i = 0; i < size1; i++)
  {
    if (i % sizex == 0 && sizey != 8) OLED_Set_Pos(x, y++);
    if (sizey == 8)
    {
      temp = pgm_read_byte(&asc2_0806[c][i]);
      OLED_WR_Byte(temp, OLED_DATA); //6X8字号
    }
    else if (sizey == 16)
    {
      temp = pgm_read_byte(&asc2_1608[c][i]);
      OLED_WR_Byte(temp, OLED_DATA); //8x16字号
    }
    else return;
  }
}

//m^n函数
u32 oled_pow(u8 m, u8 n)
{
  u32 result = 1;
  while (n--)result *= m;
  return result;
}

//显示数字
//x,y :起点坐标
//num:要显示的数字
//len :数字的位数
//sizey:字体大小
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 sizey)
{
  u8 t, temp, m = 0;
  u8 enshow = 0;
  if (sizey == 8)m = 2;
  for (t = 0; t < len; t++)
  {
    temp = (num / oled_pow(10, len - t - 1)) % 10;
    if (enshow == 0 && t < (len - 1))
    {
      if (temp == 0)
      {
        OLED_ShowChar(x + (sizey / 2 + m)*t, y, ' ', sizey);
        continue;
      } else enshow = 1;
    }
    OLED_ShowChar(x + (sizey / 2 + m)*t, y, temp + '0', sizey);
  }
}

//显示一个字符号串
void OLED_ShowString(u8 x, u8 y, const char *chr, u8 sizey)
{
  u8 j = 0;
  while (chr[j] != '\0')
  {
    OLED_ShowChar(x, y, chr[j++], sizey);
    if (sizey == 8)x += 6;
    else x += sizey / 2;
  }
}


//显示汉字
void OLED_ShowChinese(u8 x, u8 y, const u8 no, u8 sizey)
{
  u16 i, size1 = (sizey / 8 + ((sizey % 8) ? 1 : 0)) * sizey;
  u8 temp;
  for (i = 0; i < size1; i++)
  {
    if (i % sizey == 0) OLED_Set_Pos(x, y++);
    if (sizey == 16)
    {
      temp = pgm_read_byte(&Hzk[no][i]);
      OLED_WR_Byte(temp, OLED_DATA); //16x16字号
    }
    //    else if(sizey==xx) OLED_WR_Byte(xxx[c][i],OLED_DATA);//用户添加字号
    else return;
  }
}

//显示图片
//x,y显示坐标
//sizex,sizey,图片长宽
//BMP：要显示的图片
void OLED_DrawBMP(u8 x, u8 y, u8 sizex, u8 sizey, const u8 BMP[])
{
  u16 j = 0;
  u8 i, m, temp;
  sizey = sizey / 8 + ((sizey % 8) ? 1 : 0);
  for (i = 0; i < sizey; i++)
  {
    OLED_Set_Pos(x, i + y);
    for (m = 0; m < sizex; m++)
    {
      temp = pgm_read_byte(&BMP[j++]);
      OLED_WR_Byte(temp, OLED_DATA);
    }
  }
}

//OLED的初始化
void OLED_Init(void)
{
  pinMode(res, OUTPUT); //RES
  Wire.begin(0x3c); // join i2c bus (address optional for master)
  OLED_RES_Clr();
  delay(200);
  OLED_RES_Set();

  OLED_WR_Byte(0xAE, OLED_CMD); //--turn off oled panel
  OLED_WR_Byte(0x00, OLED_CMD); //---set low column address
  OLED_WR_Byte(0x10, OLED_CMD); //---set high column address
  OLED_WR_Byte(0x40, OLED_CMD); //--set start line address  Set Mapping RAM Display Start Line (0x00~0x3F)
  OLED_WR_Byte(0x81, OLED_CMD); //--set contrast control register
  OLED_WR_Byte(0xCF, OLED_CMD); // Set SEG Output Current Brightness
  OLED_WR_Byte(0xA1, OLED_CMD); //--Set SEG/Column Mapping     0xa0左右反置 0xa1正常
  OLED_WR_Byte(0xC8, OLED_CMD); //Set COM/Row Scan Direction   0xc0上下反置 0xc8正常
  OLED_WR_Byte(0xA6, OLED_CMD); //--set normal display
  OLED_WR_Byte(0xA8, OLED_CMD); //--set multiplex ratio(1 to 64)
  OLED_WR_Byte(0x3f, OLED_CMD); //--1/64 duty
  OLED_WR_Byte(0xD3, OLED_CMD); //-set display offset Shift Mapping RAM Counter (0x00~0x3F)
  OLED_WR_Byte(0x00, OLED_CMD); //-not offset
  OLED_WR_Byte(0xd5, OLED_CMD); //--set display clock divide ratio/oscillator frequency
  OLED_WR_Byte(0x80, OLED_CMD); //--set divide ratio, Set Clock as 100 Frames/Sec
  OLED_WR_Byte(0xD9, OLED_CMD); //--set pre-charge period
  OLED_WR_Byte(0xF1, OLED_CMD); //Set Pre-Charge as 15 Clocks & Discharge as 1 Clock
  OLED_WR_Byte(0xDA, OLED_CMD); //--set com pins hardware configuration
  OLED_WR_Byte(0x12, OLED_CMD);
  OLED_WR_Byte(0xDB, OLED_CMD); //--set vcomh
  OLED_WR_Byte(0x40, OLED_CMD); //Set VCOM Deselect Level
  OLED_WR_Byte(0x20, OLED_CMD); //-Set Page Addressing Mode (0x00/0x01/0x02)
  OLED_WR_Byte(0x02, OLED_CMD); //
  OLED_WR_Byte(0x8D, OLED_CMD); //--set Charge Pump enable/disable
  OLED_WR_Byte(0x14, OLED_CMD); //--set(0x10) disable
  OLED_WR_Byte(0xA4, OLED_CMD); // Disable Entire Display On (0xa4/0xa5)
  OLED_WR_Byte(0xA6, OLED_CMD); // Disable Inverse Display On (0xa6/a7)
  OLED_Clear();
  OLED_WR_Byte(0xAF, OLED_CMD); /*display ON*/
}

void ShowOLEDInfo() {
  OLED_ShowChinese(0, 0, 0, 16);  //糖
  OLED_ShowChinese(18, 0, 1, 16); //分
  OLED_ShowChinese(36, 0, 2, 16); //计
  OLED_ShowChinese(54, 0, 3, 16); //算
  OLED_ShowChinese(72, 0, 4, 16); //器
  OLED_ShowChar(95, 0, 86, 16);   //V
  OLED_ShowNum(105, 0, 1, 1, 16); //1
  OLED_ShowChar(112, 0, 46, 16);  //.
  OLED_ShowNum(118, 0, 0, 1, 16); //0

  OLED_ShowString(0, 2, "ID:", 16);
  OLED_ShowString(50, 2, "Sugar:", 16);
  OLED_ShowString(0, 4, "Name:", 16);
  OLED_ShowString(0, 6, "Sum:", 16);
  OLED_ShowNum(50, 8, sumSugarVal, 5 , 8);
}

void touchDetect() {
  if (digitalRead(touchPin)) {
    digitalWrite(Led_Y, HIGH);
    Serial.println("touched");
  } else {
    digitalWrite(Led_Y, LOW);
  }
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}


bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void initRTC()
{
  bool parse = false;
  bool config = false;

  // get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
    parse = true;
    // and configure the RTC with this info
    if (RTC.write(tm)) {
      config = true;
    }
  }

  if (parse && config) {
    Serial.print("DS1307 configured Time=");
    Serial.print(__TIME__);
    Serial.print(", Date=");
    Serial.println(__DATE__);
  } else if (parse) {
    Serial.println("DS1307 Communication Error :-{");
    Serial.println("Please check your circuitry");
  } else {
    Serial.print("Could not parse info from the compiler, Time=\"");
    Serial.print(__TIME__);
    Serial.print("\", Date=\"");
    Serial.print(__DATE__);
    Serial.println("\"");
  }
}

void sayWord(int TabIndex) {

  xfs.StartSynthesis(TextTab2[TabIndex]);
  while (xfs.GetChipStatus() != xfs.ChipStatus_Idle)
  {
    delay(30);
  }

}
