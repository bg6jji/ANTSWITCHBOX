/* TW2010手自一体控制器，支持ACC、串口和手动控制优先级从高到低分别为ACC、串口、手动。10、12、
15、17波段对应设置pin脚控制，20米默认全部为LOW。出可控波段外，全部按20米输出。未用管脚为0、1、2、3、4 */
#define DEBUG false    //璋冭瘯寮€鍏?
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x27 for a 16 chars and 2 line display

String inputString, Freq, band;   //定义变量
int Band0Pin = A0;     // K3 ACC口对应的定义引脚，左边是K3的，右边是单片机的
int Band1Pin = A1;
int Band2Pin = A2;
int Band3Pin = A3;

//int B20Pin = 4;        //波段输出引脚
int B17Pin = 5;		//如果给TW2010用,这里的20和40可以不接
int B15Pin = 6;
int B12Pin = 7;
int B10Pin = 8;
//int B40Pin = 2;

//int B20hPin = 3;        //波段输入引脚
int B17hPin = 9;		//如果给TW2010用,这里的20和40可以不接
int B15hPin = 11;
int B12hPin = 12;
int B10hPin = 13;

int AutoPin = 10;	//自动模式的控制脚，这个引脚接在波段开关的Auto档位上，LOW为启动自动模式。
int TxPin = 1;         //If TxPin is High,then the other COM is sending .
int AutoState = 0;
int TxState = 0;
int bandinfo = 0;

//int oldband = 0;
String modeinfo;

void setup() {
  pinMode(0, INPUT);
  pinMode(1, OUTPUT);
  
  
  pinMode(Band0Pin, INPUT_PULLUP);	//程序开始，设定BCD引脚为输入状态,拉高电平，防止干扰，LOW生效
  pinMode(Band1Pin, INPUT_PULLUP);
  pinMode(Band2Pin, INPUT_PULLUP);
  pinMode(Band3Pin, INPUT_PULLUP);
  pinMode(AutoPin, INPUT_PULLUP);	//设定自动模式引脚为输入

  //pinMode(B40Pin, OUTPUT);	//设定波段引脚为输出
  //pinMode(B20Pin, OUTPUT);
  pinMode(B17Pin, OUTPUT);
  pinMode(B15Pin, OUTPUT);
  pinMode(B12Pin, OUTPUT);
  pinMode(B10Pin, OUTPUT);

  //pinMode(B20hPin, INPUT_PULLUP);//设定波段引脚为输入,拉高电平，防止干扰，LOW生效
  pinMode(B17hPin, INPUT_PULLUP);
  pinMode(B15hPin, INPUT_PULLUP);
  pinMode(B12hPin, INPUT_PULLUP);
  pinMode(B10hPin, INPUT_PULLUP);

  lcd.init();                      // initialize the lcd
  lcd.backlight();//打开背光灯
  lcd.noCursor();
  lcd.setCursor(0, 0);
  lcd.print("Design By BG6JJI");
  lcd.setCursor(0, 1);
  lcd.print("Made   By BG8NUD");
  delay (2000);
  lcd.clear();//不能老清屏，屏幕会闪的厉害

  Serial.begin(9600,SERIAL_8N1 );		//启动串口,速率38400，可调整
    Serial.println("AI3;");      //这里是双向模式用的设定Kx3的命令，单向暂时不需要
}

void loop() {
  AutoState = digitalRead(AutoPin);         //不停读自动模式脚状态
  if (AutoState == LOW) {     		//如果自动模式电平为低，则进入自动切换模式

    if (accbandcode() != 1111) {    //拉高引脚，如果为LOW说明有信号输入，参见后续ACC部分说明
      BandSwitch(accbandcode());    //按accbandcode代码切换波段
      modeinfo = "ACC";
      displaybandinfo();
    }
    else   {                       //否则使用串口切换波段
//      pinMode(TxPin, INPUT);
  //    TxState = digitalRead(TxPin);
 //     delay(10);
  //    pinMode(TxPin, OUTPUT );
//      if (TxState == HIGH ) {
        
//      } else {
     

//  delay(200);
//  Serial.println("IF;");
 
//      }
      Serialpress(); //处理串口信息
      BandSwitch(cmdprocess());
      modeinfo = "COM";
      displaybandinfo();
    }
  }
  else {
    BandSwitch(handswitch());			//如果自动状态不满足，进入手动模式，由手动控制码控制，继电器按手动开关位置动作。
    modeinfo = "MT ";
    displaybandinfo();
  }

}

int handswitch() {
  delay (600);   //延时，软件消抖用，不加则在开关切换过程中会回到20米，也可以硬件开关两端并联0.1UF电容实现硬件防抖。硬件部分本次暂不采用。
  //if ( digitalRead(B40hPin)==LOW ) return 1; else   //upthere 40m
  //if (digitalRead(B20hPin) == LOW  )    return 2; else  //upthere 20m
  if (digitalRead(B17hPin) == LOW  )  return 3; else  //upthere 17m 按引脚状态返回波段代码，都不满足返回99即默认20米
      if (digitalRead(B15hPin) == LOW  )  return 4; else  //upthere 15m
          if (digitalRead(B12hPin) == LOW  )  return 5; else  //upthere 12m
              if (digitalRead(B10hPin) == LOW  )  return 6; else  //upthere 10m
          // if (digitalRead(B6hPin)== LOW )  return 7; else
          return 99;   //upthere 6m
}

void displaybandinfo() {

  //lcd.clear();//不能老清屏，屏幕会闪的厉害
  lcd.noCursor();//隐藏光标
  lcd.setCursor(5, 0); //光标起始位置第1行第8列
  lcd.print("BG8NUD");// 用户呼号

  lcd.setCursor(0, 1); //光标起始位置第2行第1列
  lcd.print("BAND:" );//显示当前波段
  lcd.setCursor(8, 1);
  lcd.print(bandinfo);
  lcd.print("M");
  lcd.setCursor(13, 1); //光标起始位置第2行第1列
  //lcd.print("MODE:" );// 显示模式状态
  lcd.print(modeinfo);
}


int  FreeRam() {				//内存剩余容量子程序
  extern int  __bss_end;
  extern int* __brkval;
  int free_memory;
  if (reinterpret_cast<int>(__brkval) == 0) {
    // if no heap use from end of bss section
    free_memory = reinterpret_cast<int>(&free_memory)
                  - reinterpret_cast<int>(&__bss_end);
  } else {
    // use from top of stack to heap
    free_memory = reinterpret_cast<int>(&free_memory)
                  - reinterpret_cast<int>(__brkval);
  }
  return free_memory;
}

void Serialpress() {				//处理串口信息子程序
  while (Serial.available()>0) {
    // get the new byte:
      inputString +=char( Serial.read());
//  #ifdef DEBUG
//    Serial.print(inputString);
//#endif
    delay(2);
  }
}

void BandSwitch(int _band) {			//切换波段子程序，数字对应波段
//#ifdef DEBUG
//  Serial.println("BandSwitch:" + _band);
//#endif
  switch (_band) {
    //    case 1:
    //40m Band
    //      digitalWrite(B20Pin, LOW);
    //      digitalWrite(B17Pin, LOW);
    //      digitalWrite(B15Pin, LOW);
    //      digitalWrite(B12Pin, LOW);
    //      digitalWrite(B10Pin, LOW);
    //      digitalWrite(B40Pin, HIGH);
    //      bandinfo = 40;
    //      break;
    //case 2:
    //20m Band
    //digitalWrite(B20Pin, HIGH);
    //digitalWrite(B17Pin, LOW);
    //digitalWrite(B15Pin, LOW);
    //digitalWrite(B12Pin, LOW);
    // digitalWrite(B10Pin, LOW);
    //      digitalWrite(B40Pin, LOW);
    //bandinfo = 20;
    // break;
    case 3:
      //17m Band
      //digitalWrite(B20Pin, LOW);
      digitalWrite(B17Pin, HIGH);
      digitalWrite(B15Pin, LOW);
      digitalWrite(B12Pin, LOW);
      digitalWrite(B10Pin, LOW);
      //      digitalWrite(B40Pin, LOW);
      bandinfo = 17;  //返回波段信息，在LCD显示用。下同
      break;
    case 4:
      //15m Band
      //digitalWrite(B20Pin, LOW);
      digitalWrite(B17Pin, LOW);
      digitalWrite(B15Pin, HIGH);
      digitalWrite(B12Pin, LOW);
      digitalWrite(B10Pin, LOW);
      //digitalWrite(B40Pin, LOW);
      bandinfo = 15;
      break;
    case 5:
      //12m Band
      //digitalWrite(B20Pin, LOW);
      digitalWrite(B17Pin, LOW);
      digitalWrite(B15Pin, LOW);
      digitalWrite(B12Pin, HIGH);
      digitalWrite(B10Pin, LOW);
      //      digitalWrite(B40Pin, LOW);
      bandinfo = 12;
      break;
    case 6:
      //10m Band
      //digitalWrite(B20Pin, LOW);
      digitalWrite(B17Pin, LOW);
      digitalWrite(B15Pin, LOW);
      digitalWrite(B12Pin, LOW);
      digitalWrite(B10Pin, HIGH);
      //      digitalWrite(B40Pin, LOW);
      bandinfo = 10;
      break;
    case 999:
          break;
    default:  //所有不满足条件的都输出20米控制信号
      //20m Band
      //digitalWrite(B20Pin, HIGH);
      digitalWrite(B17Pin, LOW);
      digitalWrite(B15Pin, LOW);
      digitalWrite(B12Pin, LOW);
      digitalWrite(B10Pin, LOW);
      //      digitalWrite(B40Pin, LOW);
      bandinfo = 20;
      break;
  }
}

int accbandcode() {			//处理BCD码，下面有对应状态
  /*  BAND CODE
  BAND    BAND3/2/1/0
  160m    0001
  80m     0010
  60m     0000
  40m     0011
  30m     0100
  20m     0101
  17m     0110
  15m     0111
  12m     1000
  10m     1001
  6m      1010
  */

  String bcdcode = String(digitalRead(Band3Pin)) + String(digitalRead(Band2Pin)) + String(digitalRead(Band1Pin)) + String(digitalRead(Band0Pin));	//读取每个脚状态，并合并
  int code = bcdcode.toInt();	//转换为整数去判断
//#ifdef DEBUG
//  Serial.println("ACC Band Code:" + code);
//#endif
  switch (code) {
    case 1:   //160m
      return 89;
      break;
    case 10:  //80m
      return 88;
      break;
    case 0:   //60m
      return 87;
      break;
    case 11:  //40m
      return 1;
      break;
    case 100:  //30m
      return 86;
      break;
    case 101:  //20m
      return 2;
      break;
    case 110:  //17m
      return 3;
      break;
    case 111:  //15m
      return 4;
      break;
    case 1000:  //12m
      return 5;
      break;
    case 1001:  //10m
      return 6;
      break;
    case 1010:  //6m
      return 7;
      break;
    case 1111: // no input return 1111
      return 1111;
      break;
    default:
      return 99;
      break;
  }

}

int cmdprocess() {					//串口命令判断子程序
  while (inputString.length() > 0) {
    int indexofcmd = inputString.indexOf(';');	//判断是否为空
    if ( indexofcmd < 0)  {
      inputString = "";
//#ifdef DEBUG
//      Serial.print("cmdprocess():IndexOf ; < 0.Breaking...");
//#endif
        break;
    }
    String singlecmd = inputString.substring(0, indexofcmd);
    inputString = inputString.substring(indexofcmd + 1, inputString.length());
//#ifdef DEBUG
//    Serial.print("Singlecmd:"); Serial.println(singlecmd);
//    Serial.print("inputString:"); Serial.println(inputString);
//#endif
    if (singlecmd.startsWith("FA")) return (stringtoband(singlecmd.substring(5, 7)));		//如果命令起始FA则执行取波段操作
    else 
    if (singlecmd.startsWith("IF")) return (stringtoband(singlecmd.substring(5, 7)));
  		//如果命令起始IF则执行取波段操作

  }
  return 999;
}


int stringtoband(String bandstr) {   //IF[f]*****+yyyyrx*00tmvspbd1*;          //字符串转换为波段数据子程序
  int bandnum = bandstr.toInt();							//抓到的字符串转换为整数，目前只抓兆单位
  if (bandnum == 7 ) return 1; else   //upthere 40m
      if (bandnum == 14 )   return 2; else   //upthere 20m
          if (bandnum == 18 )  return 3; else   //upthere 17m
              if (bandnum == 21 )  return 4; else   //upthere 15m
                  if (bandnum == 24 )  return 5; else   //upthere 12m
                      if (bandnum == 28 || bandnum == 29 )  return 6; else   //upthere 10m
                          if (bandnum == 51 )  return 7; else return 99;   //upthere 6m
//#ifdef DEBUG
//  Serial.println("Serial_Bandnumber:" + bandnum);
//#endif
}

