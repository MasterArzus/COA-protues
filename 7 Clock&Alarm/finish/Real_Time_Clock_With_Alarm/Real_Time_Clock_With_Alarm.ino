#include <DS3231.h>//Memanggil RTC3231 Library
#include <Wire.h>  // i2C Conection Library
#include <LiquidCrystal.h> //Libraries
#include <EEPROM.h>
#include <TimeLib.h>
/*---------------------------------------参数设置-------------------------------------------------*/
//Define Pin
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); //Arduino pins to lcd

//当开关闭合后，将获得低电平开关信号0
#define bt_time   A0
#define bt_up     A1
#define bt_down   A2
#define bt_alarm  A3

#define buzzer 8

// Init DS3231
DS3231  rtc(SDA, SCL);

// Init a Time-data structure
Time  t; //Which define in DS3231.h

// 时间参数
int yy = 0, hh = 0, mm = 0, ss = 0, dd = 0, mo = 0, week_day;
String Day = "  ";

// 闹钟参数
int AlarmHH  = 0, AlarmMM  = 0, AlarmSS  = 0;

// Counter
int set_timer_counter = 0, set_alarm_counter = 0, set_counter_counter = 0;

// Flags
int IF_WRITE_TO_EEPROM =0, IF_ALARM_OPENED=0, SETTING=0, IF_ALARM_SETTED=0;

//Eeprom Store Variable
uint8_t HH;
uint8_t MM;

// 倒计时参数
int IF_PRESSED_UP=0, IF_PRESSED_DOWN=0;
long cdExpectedTime = 0, cdDifferenceTime = 0;
int cdHH = 0, cdMM = 0, cdSS = 0;
int cdStop = 0, cdDigit = 1;
int interfaceId = 0;

long pressStartTime, pressTime;
int pressBtn, press, pressCheckSum;

/*字符取模
当希望显示字库之外的字符时,先给字符取模
创建8个自定义字符（注意这8个自定义字符从0开始编号！！）
每个自定义字符的外观由八个字节的数组决定，每行占用一个字符。
所在的行最低由五个有效的像素点决定。
数组名称为这个字模数据的名称（data）
0表示这个像素点为空，1表示这个像素点有显示
*/

// 字符的像素数据
byte bell_symbol[8] = {
  B00100,
  B01110,
  B01110,
  B01110,
  B01110,
  B11111,
  B01000,
  B00100};

byte thermometer_symbol[8] = {
  B00100,
  B01010,
  B01010,
  B01110,
  B11111,
  B11011,
  B01110,
  B01110};

byte degree_symbol[8] = {
  B01110,
  B01010,
  B01110,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000};
/*---------------------------------------其他函数-------------------------------------------------*/

//BLINKING SCREEN
void blinking(){
  if (set_counter_counter==0){
    //间隔为2，不包括Curser位置
    if (set_alarm_counter ==0 && set_timer_counter == 1){lcd.setCursor(0,1);  lcd.print("  ");}
    if (set_alarm_counter ==0 && set_timer_counter == 2){lcd.setCursor(3,1);  lcd.print("  ");}
    if (set_alarm_counter ==0 && set_timer_counter == 3){lcd.setCursor(6,1);  lcd.print("  ");}

    if (set_alarm_counter ==0 && set_timer_counter == 4){lcd.setCursor(1,0);  lcd.print("   ");}
    if (set_alarm_counter ==0 && set_timer_counter == 5){lcd.setCursor(5,0);  lcd.print("  ");}
    if (set_alarm_counter ==0 && set_timer_counter == 6){lcd.setCursor(8,0);  lcd.print("  ");}
    if (set_alarm_counter ==0 && set_timer_counter == 7){lcd.setCursor(11,0); lcd.print("    "); }

    //Alarm
    if (set_timer_counter == 0 && set_alarm_counter == 1){lcd.setCursor(6,0); lcd.print("           "); }
    if (set_timer_counter == 0 && set_alarm_counter == 2){lcd.setCursor(4,1); lcd.print("  "); }
    if (set_timer_counter == 0 && set_alarm_counter == 3){lcd.setCursor(7,1); lcd.print("  "); }
    if (set_timer_counter == 0 && set_alarm_counter == 4){lcd.setCursor(10,1);lcd.print("  "); }
  }
}

//设置闹钟显示
void alarmUpdate (){
  if (set_timer_counter == 0 && set_alarm_counter >0){
    lcd.setCursor (0,0);
    lcd.print("Alarm ");

    if(IF_ALARM_OPENED==0){lcd.print("Deactivate");}
    else{lcd.print("Activated ");}
        
    lcd.setCursor (4,1);
    lcd.print((AlarmHH/10)%10);
    lcd.print(AlarmHH % 10);
    lcd.print(":");
    lcd.print((AlarmMM/10)%10);
    lcd.print(AlarmMM % 10);
    lcd.print(":");
    lcd.print((AlarmSS/10)%10);
    lcd.print(AlarmSS % 10);
 }
}

void ReadEeprom () {
  AlarmHH=EEPROM.read(1);
  AlarmMM=EEPROM.read(2);
  AlarmSS=EEPROM.read(3);
  
  IF_ALARM_OPENED=EEPROM.read(4); 

//读入错误
  cdHH = EEPROM.read(5);
  cdMM = EEPROM.read(6);
  cdSS = EEPROM.read(7);
}
  /***
    Write the value to the appropriate byte of the EEPROM.
    these values will remain there when the board is
    turned off.
    EEPROM.write(addar,val);
  ***/
void WriteEeprom () {
  EEPROM.write(1,AlarmHH);
  EEPROM.write(2,AlarmMM);
  EEPROM.write(3,AlarmSS);

  EEPROM.write(4,IF_ALARM_OPENED);

  EEPROM.write(5,cdHH);
  EEPROM.write(6,cdMM);
  EEPROM.write(7,cdSS);
}

void boundChech(){
  if(hh>23) hh=0;
  if(mm>59) mm=0;
  if(ss>59) ss=0;
  if(week_day>7) week_day=0;
  if(dd>31) dd=0;
  if(mo>12) mo=0;
  if(yy>2030) yy=2000;
}

void cdDiffToDigit()
{
  cdHH = cdDifferenceTime / 3600000;
  cdMM = (cdDifferenceTime % 3600000) / 60000;
  cdSS = (cdDifferenceTime % 60000) / 1000;
}

void cdConfirm()
{
    cdDifferenceTime = cdHH * 3600000 + cdMM * 60000 + cdSS * 1000;
    cdExpectedTime = millis() + cdDifferenceTime;
}

void cdShowHeader()
{
  lcd.setCursor(0, 0);
  lcd.print(F("   Countdown"));
}

void cdShow()
{
  lcd.setCursor(0, 1);
  lcd.print("    ");

  if (cdHH < 10)
  {
    lcd.print("0");
  }
  lcd.print(cdHH);
  lcd.print(":");

  if (cdMM < 10)
  {
    lcd.print("0");
  }
  lcd.print(cdMM);
  lcd.print(":");

  if (cdSS < 10)
  {
    lcd.print("0");
  }
  lcd.print(cdSS);
  lcd.print("    ");
}

void cdHide(){
  lcd.setCursor(1 + 3 * cdDigit, 1);
  lcd.print("  ");
}

void cdIndicator(){
  lcd.setCursor(1 + 3 * cdDigit, 0);
  lcd.print("__");
}

void cdToggle(){
  cdStop = (cdStop == 1) ? 0 : 1;
  if (!cdStop)
    cdExpectedTime = millis() + cdDifferenceTime;

  cdShow();
  delay(300);
}

void cdLogicFlow(){
  //运行计算
  t = rtc.getTime();
  if (cdDifferenceTime > 0) {
    if (cdStop == false) // Running
      cdDifferenceTime = cdExpectedTime - millis();
  }

  //显示逻辑
  if (interfaceId == 4) {
    cdShowHeader();

    if (cdDifferenceTime <= 0){
      if (!cdStop){// Half stop
        cdStop = true;
        cdDifferenceTime = 0;

        digitalWrite(buzzer, LOW);
        lcd.setCursor(0, 1);
        lcd.print(F(" Counter Done!  "));
        delay(3000);

        digitalWrite(buzzer, HIGH);
        interfaceId = 5;
        lcd.setCursor(0, 0);
        lcd.print("               ");
      }
    }

    cdDiffToDigit();
    cdShow();
  }

  if (interfaceId == 5){
    cdIndicator();
    cdShow();
  }
}

void cdIncr(){
    switch (cdDigit){
      case 3: // cdSS
        if (cdSS < 59){
          cdSS++;
          break;
        }
        else
          cdSS = 0; // Continue to incr cdMM
      case 2:           // cdMM
        if (cdMM < 59){
          cdMM++;
          break;
        }
        else
          cdMM = 1; // Continue to incr cdHH
      case 1:           // cdHH
        if (cdHH < 72)
          cdHH++;
        break;
      default:
          break;
    }
}

void cdDecr(){
  switch (cdDigit){
    case 3: // cdSS
      if (cdSS > 0){
        cdSS--;
        break;
      }
      else
        cdSS = 59; // Continue to decr cdMM
    case 2:            // cdMM
      if (cdMM > 0){
        cdMM--;
        break;
      }else
        cdMM = 59; // Continue to decr cdHH
    case 1:            // cdHH
      if (cdHH > 0)
        cdHH--;
      break;
    default:
      break;
  }
}

void shortPressLogicFlow(){
  switch (interfaceId){
  case 4: // Countdown
    switch (pressBtn){
      case bt_time:
        break;
      case bt_up:
      case bt_down:
        cdToggle();
        break;
      case bt_alarm:
        break;
    }
    break;
  case 5: // Countdown Setting
    switch (pressBtn){
    case bt_time:
      cdConfirm();
      interfaceId = 4;
      cdStop = false;
      cdDigit = 1;
      break;
    case bt_up:
      cdIncr();
      break;
    case bt_down:
      cdDecr();
      break;
    case bt_alarm:
      if(cdDigit < 3){
        cdDigit++;
        lcd.setCursor(0, 0);
        lcd.print("               ");
        cdIndicator();
      }else{//退出counter
        set_counter_counter = 0;
        IF_PRESSED_UP=0;
        IF_PRESSED_DOWN=0;
      } 
      break;
    }
    break;
  }

  pressBtn=0;
}

void longPressLogicFlow(){
  switch (pressBtn){
    case bt_up:
    case bt_down:
      if (interfaceId == 4){
        cdStop = true;
        cdDifferenceTime = 0;
        cdDiffToDigit();
        cdConfirm();
        interfaceId = 5;
        lcd.setCursor(0, 0);
        lcd.print("               ");
      }
      break;
  }
}

//按下按键，调整显示
void update () {
  //按8下，完成时钟设置
  if (set_timer_counter == 8){
    lcd.setCursor (0,0);
    lcd.print (F("Set Date Finish "));
    lcd.setCursor (0,1);
    lcd.print (F("Set Time Finish "));

    delay (1000);

    rtc.setTime (hh, mm, ss);
    rtc.setDate (dd, mo, yy);  
    
    lcd.clear();
    set_timer_counter = 0;
  }
  //按5下，完成闹钟设置
  if (set_alarm_counter == 5){
    lcd.setCursor (0,0);
    lcd.print (F("Set Alarm Finish"));
    lcd.setCursor (0,1);
    lcd.print (F("-EEPROM Updated-"));
    WriteEeprom();

    delay (2000); 

    lcd.clear();
    set_alarm_counter=0;

    //表示闹钟完成设置参数
    IF_ALARM_SETTED=1;
  }
  
  //还原alarmMode为0，表示闹钟未完成设置
  if (set_alarm_counter >0){ IF_ALARM_SETTED=0;}
  
  //按下SET_TIME，
  if(digitalRead (bt_time) == 0 && SETTING==0) {
    if(set_counter_counter == 0){
      SETTING=1;//设置flag为1，表示设置状态
      if(set_alarm_counter>0){set_alarm_counter=5;}//再按一下SET_ALARM，马上结束设置
      else{set_timer_counter = set_timer_counter+1;}//设置不同的数位
    }else{//开始倒计时----------------
    }
  }

  //按下SET_ALARM，
  if(digitalRead (bt_alarm) == 0 && SETTING==0){ 

    if(IF_PRESSED_UP==1 && IF_PRESSED_DOWN==1){//切换倒计时设置
      set_counter_counter=set_counter_counter+1;
    }

    if(set_counter_counter == 0){
      SETTING=1;//必须放在counter设置后面
      if(set_timer_counter>0){set_timer_counter=8;}//再按一下SET_TIME，马上结束设置
      else{set_alarm_counter = set_alarm_counter+1;} 
      lcd.clear();//不清理不行,否则会有残余显示
    }
  } 

  //还原flag为0，表示普通显示状态
  if(digitalRead (bt_time) == 1 && digitalRead (bt_alarm) == 1){SETTING=0;}

  //按下UP键
  if(digitalRead (bt_up) == 0){         
    //counter
    if(set_alarm_counter==0 && set_timer_counter==0 && IF_PRESSED_UP==0){
      IF_PRESSED_UP=1;
      if(IF_PRESSED_DOWN==1){
        set_counter_counter=1;

        //显示count界面
        interfaceId = 5;
        cdDigit = 1;
        lcd.clear();
      } 
    } 

    if (set_counter_counter==0){
      //Timer
      if (set_alarm_counter==0 && set_timer_counter==1)hh=hh+1; 
      if (set_alarm_counter==0 && set_timer_counter==2)mm=mm+1;
      if (set_alarm_counter==0 && set_timer_counter==3)ss=ss+1;
      if (set_alarm_counter==0 && set_timer_counter==4)week_day=week_day+1;
      if (set_alarm_counter==0 && set_timer_counter==5)dd=dd+1;
      if (set_alarm_counter==0 && set_timer_counter==6)mo=mo+1;
      if (set_alarm_counter==0 && set_timer_counter==7)yy=yy+1;
      rtc.setDOW(week_day);

      //Alarm
      if (set_timer_counter==0 && set_alarm_counter==1) IF_ALARM_OPENED=1;//打开闹钟，允许闹铃
      if (set_timer_counter==0 && set_alarm_counter==2 && AlarmHH<24) AlarmHH=AlarmHH+1;
      if (set_timer_counter==0 && set_alarm_counter==3 && AlarmMM<59) AlarmMM=AlarmMM+1;
      if (set_timer_counter==0 && set_alarm_counter==4 && AlarmSS<59) AlarmSS=AlarmSS+1;
    }
    
    boundChech();
 }        

  //按下DOWN键
  if(digitalRead (bt_down) == 0){        
    //counter
    if(set_alarm_counter==0 && set_timer_counter==0 && IF_PRESSED_DOWN==0){
      IF_PRESSED_DOWN=1;
      if(IF_PRESSED_UP==1){
        set_counter_counter=1;
        
        //显示count界面
        interfaceId = 5;
        cdDigit = 1;
        lcd.clear();
      }
    } 

    if (set_counter_counter==0){
      //Timer
      if (set_alarm_counter==0 && set_timer_counter==1)hh=hh-1; 
      if (set_alarm_counter==0 && set_timer_counter==2)mm=mm-1;
      if (set_alarm_counter==0 && set_timer_counter==3)ss=ss-1;
      if (set_alarm_counter==0 && set_timer_counter==4)week_day=week_day-1;
      if (set_alarm_counter==0 && set_timer_counter==5)dd=dd-1;
      if (set_alarm_counter==0 && set_timer_counter==6)mo=mo-1;
      if (set_alarm_counter==0 && set_timer_counter==7)yy=yy-1;
      rtc.setDOW(week_day);

      //Alarm
      if (set_timer_counter==0 && set_alarm_counter==1 )IF_ALARM_OPENED=0;
      if (set_timer_counter==0 && set_alarm_counter==2 && AlarmHH>0)AlarmHH=AlarmHH-1;
      if (set_timer_counter==0 && set_alarm_counter==3 && AlarmMM>0)AlarmMM=AlarmMM-1;
      if (set_timer_counter==0 && set_alarm_counter==4 && AlarmSS>0)AlarmSS=AlarmSS-1;
    }
  
    //Bound Chech
    boundChech();
  }

  //判断counter显示
  if(IF_PRESSED_UP==1 && IF_PRESSED_DOWN==1){
    counterUpdate();
  }
}

void counterUpdate(){
  // 四个按键，用 pressCheckSum 来判断哪个按键被按下
  pressCheckSum = 4 * digitalRead(bt_time) + 3 * digitalRead(bt_up) + 2 * digitalRead(bt_down) + 1 * digitalRead(bt_alarm);

  if (pressCheckSum != 10 && press == 0){
    press = 1;
    switch (pressCheckSum){
      case 6:
        pressBtn = bt_time;
        break;
      case 7:
        pressBtn = bt_up;
        break;
      case 8:
        pressBtn = bt_down;
        break;
      case 9:
        pressBtn = bt_alarm;
        break;
    }

    pressStartTime = millis();
  }

  if (pressCheckSum == 10 && press == 1){
    pressTime = millis() - pressStartTime;

    if (pressTime < 800){ 
      shortPressLogicFlow();
    }
    else{ 
      longPressLogicFlow();
    }
    
    press = 0;
  }

  cdLogicFlow();
}

/*---------------------------------------主要函数-------------------------------------------------*/
/*主要函数
草图启动时会调用 setup()函数。
使用它来初始化变量，引脚模式，启用库等。
setup函数只能在Arduino板的每次上电或复位后运行一次。*/
void setup(){
  // Setup Serial connection，以每秒9600位的速度初始化串行连接
  Serial.begin(9600);
  rtc.begin(); // Initialize the rtc object

  /*将Arduino的引脚配置为以下三种模式：
  Arduino将开启引脚的内部上拉电阻，实现上拉输入功能。
  一旦将引脚设置为输入（INPUT）模式，Arduino内部上拉电阻将被禁用。*/
  pinMode(bt_time,  INPUT_PULLUP);
  pinMode(bt_up,    INPUT_PULLUP);
  pinMode(bt_down,  INPUT_PULLUP);
  pinMode(bt_alarm, INPUT_PULLUP);

  pinMode(buzzer, OUTPUT);

  /*声明用户自定义字符
  使用自定义字符前，我们需要先进行创建自定义字符，
  让机器知道我们的自定义字符。创建自定义字符在arduino的 setup() { } 函数中，
  使用lcd.createChar()方法
  */
  lcd.createChar(1, thermometer_symbol);//自定义字符编号为1,2
  lcd.createChar(2, bell_symbol);
  lcd.createChar(3, degree_symbol);

  lcd.begin(16, 2); // Initializes the interface to the LCD screen, 
  //and specifies the dimensions (width and height) of the display

  //----------------------------------------开场显示
  //定位LCD光标位置, lcd .setCursor（col，row）
  lcd.setCursor(0,0);  //Show "TIME" on the LCD
  lcd.print(" Real Time Clock ");

  lcd.setCursor (0,1);
  lcd.print("With Alarm & CD");

  // Wait two second before repeating
  delay (2000);
  lcd.clear();

  // read a byte from the current address（50） of the EEPROM
  IF_WRITE_TO_EEPROM=EEPROM.read(50);
  if(IF_WRITE_TO_EEPROM!=0){WriteEeprom ();}//写入alarm参数
  EEPROM.write(50,0); 

  //读取alarm参数
  ReadEeprom();

  //计算倒计时
  cdConfirm();
}

/*主要函数
允许程序连续循环的更改和响应*/
void loop(){ //-----------------------------不断刷新显示
  //从RTC中获取当前时间
  t = rtc.getTime();
  //获取星期几，参数1，表示以缩写形式
  Day = rtc.getDOWStr(1);

  //当set_timer_counter为0时，根据RTC更新时间参数
  //DEC，英文全称 Decimal，表示十进制。
  if (set_timer_counter == 0){
    hh = t.hour,DEC;
    mm = t.min,DEC;
    ss = t.sec,DEC;
    dd = t.date,DEC;
    mo = t.mon,DEC;
    yy = t.year,DEC;
  }  

  //时钟模式，根据时间参数显示当前时间
  if(set_alarm_counter==0 && set_counter_counter==0){
    lcd.setCursor(0,1); 
    //一位一位显示时间
    lcd.print((hh/10)%10);//显示十位
    lcd.print(hh % 10); //显示个位
    lcd.print(":");

    lcd.print((mm/10)%10);
    lcd.print(mm % 10);
    lcd.print(":");

    lcd.print((ss/10)%10);
    lcd.print(ss % 10);
    lcd.print(" ");  

    //显示自定义字符，字符编号为(画出温度计和闹铃图像)
    if(IF_ALARM_OPENED==1){lcd.write(2);}
    else{lcd.print(" ");}   

    lcd.print(" "); 
    
    lcd.write(1); 
    lcd.print(rtc.getTemp(),0);

    lcd.write(3);//对应“°”
    //lcd.write(223);//？？？
    lcd.print("C");
    lcd.print("  "); 

    //换行，显示日期
    lcd.setCursor(1,0);
    lcd.print(Day);
    lcd.print(" ");
    lcd.print((dd/10)%10);
    lcd.print(dd % 10); 
    lcd.print("/");
    lcd.print((mo/10)%10);
    lcd.print(mo % 10);
    lcd.print("/"); 
    lcd.print((yy/1000)%10);
    lcd.print((yy/100)%10);
    lcd.print((yy/10)%10);
    lcd.print(yy % 10);
  }

  //动态更新各参数
  update();
  if (set_counter_counter==0) alarmUpdate();

  delay (100);

  blinking();

  //Alarming for 60s，低电平驱动
  if (IF_ALARM_SETTED==1 && IF_ALARM_OPENED==1 
  && hh==AlarmHH && mm==AlarmMM && ss>=AlarmSS) {
    digitalWrite(buzzer, LOW);
    delay (300);
    digitalWrite(buzzer, HIGH);
  }else{digitalWrite(buzzer, HIGH);}

  delay (100);
}