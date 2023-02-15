#include <Arduino.h>
/*配网*/
#include <ESP8266WiFi.h>          
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
/*TFT库*/
#include <TFT_eSPI.h>
#include <SPI.h> 
/*模库*/
#include "MyFonts.h"//导入字库        
#include "Pic_Connect.h"//导入连接图片
#include "Pic_Weather.h"
/*时间库*/
#include <NTPClient.h>
/*解析库*/
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Ticker.h>//定时获取天气
/*初始化*/
TFT_eSPI tft = TFT_eSPI(); //屏幕初始化
//Ticker weather_t;//定时器初始化
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"ntp.sjtu.edu.cn");
/*定义*/
unsigned char tick= 1;
short int flag = 1; //开机
char *weekDays[7]={"周日", "周一", "周二","周三", "周四", "周五", "周六"};
const char* host = "api.seniverse.com";
char determineqing[]="晴";
char determineduoyun[]="多云";
char determineyin[]="阴";
char determineyu[]="雨";
char determinexue[]="雪";
String now_address="",now_time="",now_temperature="",now_weather="";//用来存储报文得到的字符串
char* now_wea;//当前天气
int ph;//天气图片
/*单个汉字*/
void drawSingleWord(int32_t x,int32_t y,const char c[3],uint32_t color){
  for (int k = 0; k < 26; k++)// 根据字库的字数调节循环的次数
    if (pgm_read_byte((char *)&hanzi[k].Index[0]) == c[0] && pgm_read_byte((char *)&hanzi[k].Index[1]) == c[1] && pgm_read_byte((char *)&hanzi[k].Index[2]) == c[2])
    { 
        tft.drawBitmap(x, y, hanzi[k].hz_Id, 16, 16, color);
    }
}
/*多个汉字*/
void drawSentences(int32_t x,int32_t y,const char str[],uint32_t color)
{
//显示整句汉字，字库比较简单，上下、左右输出是在函数内实现
  int x0=x;
  for(int i=0;i<strlen(str);i+=3)
  {
    drawSingleWord(x0,y,str+i,color);
    x0 += 17;
  }
}

/*******************整句字符串显示****************/
void showtext(int16_t x,int16_t y,uint8_t font,uint8_t s,uint16_t fg,uint16_t bg,const String str)
{
  //设置文本显示坐标，和文本的字体，默认以左上角为参考点，
    tft.setCursor(x, y, font);
  // 设置文本颜色为白色，文本背景黑色
    tft.setTextColor(fg,bg);
  //设置文本大小，文本大小的范围是1-7的整数
    tft.setTextSize(s);
  // 设置显示的文字，注意这里有个换行符 \n 产生的效果
    tft.println(str);
}

void get_weather()
{
  //创建TCP连接
    WiFiClient client;
    const int httpPort = 80;
    if (!client.connect(host, httpPort))
    {
      Serial.println("connection failed");  //网络请求无响应打印连接失败
      return;
    }
    //URL请求地址
    String url ="/v3/weather/now.json?key=SdJyYxU-mQ2M8XgBZ&location=lishui&language=zh-Hans&unit=c";
    //发送网络请求
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
              "Host: " + host + "\r\n" +
              "Connection: close\r\n\r\n");
    delay(2000);
    //定义answer变量用来存放请求网络服务器后返回的数据
    String answer;
    while(client.available())
    {
      String line = client.readStringUntil('\r');
      answer += line;
    }
    //断开服务器连接
  client.stop();
  Serial.println();
  Serial.println("closing connection");
  //获得json格式的数据
  String jsonAnswer;
  int jsonIndex;
  //找到有用的返回数据位置i 返回头不要
  for (int i = 0; i < answer.length(); i++) {
    if (answer[i] == '{') {
      jsonIndex = i;
      break;
    }
  }
  jsonAnswer = answer.substring(jsonIndex);
  Serial.println();
  Serial.println("JSON answer: ");
  Serial.println(jsonAnswer);
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 210;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, jsonAnswer);
  JsonObject results_0 = doc["results"][0]; 
  JsonObject results_0_location = results_0["location"];
  const char* results_0_location_id = results_0_location["id"]; // "WX4FBXXFKE4F"
  const char* results_0_location_name = results_0_location["name"]; // "北京"
  const char* results_0_location_country = results_0_location["country"]; // "CN"
  const char* results_0_location_path = results_0_location["path"]; // "北京,北京,中国"
  const char* results_0_location_timezone = results_0_location["timezone"]; // "Asia/Shanghai"
  const char* results_0_location_timezone_offset = results_0_location["timezone_offset"]; // "+08:00"
  JsonObject results_0_now = results_0["now"];
  const char* results_0_now_text = results_0_now["text"]; // "多云"
  const char* results_0_now_code = results_0_now["code"]; // "4"
  const char* results_0_now_temperature = results_0_now["temperature"]; // "5"
  const char* results_0_last_update = results_0["last_update"]; // "2020-11-19T19:00:00+08:00"
  Serial.print("city:name:");
  Serial.println(results_0_location_name);
  now_temperature=results_0_now_temperature;
  Serial.println(now_temperature);
  now_time=results_0_last_update;
  Serial.println(now_time);
  now_weather=results_0_now_text;
  if(strstr(now_weather.c_str(),determineqing)!=0)
  {  now_wea = "晴";
     ph = 0;
  }
  if(strstr(now_weather.c_str(),determineduoyun)!=0)
  {  now_wea = "云";
     ph = 1;
  }
  if(strstr(now_weather.c_str(),determineyin)!=0)
  {  now_wea = "阴";
     ph = 2;
  }
  if(strstr(now_weather.c_str(),determineyu)!=0)
  {  now_wea = "雨";
     ph = 3;
  }
  if(strstr(now_weather.c_str(),determinexue)!=0)
  {  now_wea = "雪";
     ph = 4;
  }
}

void wifi_connect(){
  WiFiManager wifiManager;
    
    // 自动连接WiFi。以下语句的参数是连接ESP8266时的WiFi名称
    wifiManager.autoConnect("AutoConnectAP");
    
    // 如果您希望该WiFi添加密码，可以使用以下语句：
    // wifiManager.autoConnect("AutoConnectAP", "12345678");
    // 以上语句中的12345678是连接AutoConnectAP的密码
    
    // WiFi连接成功后将通过串口监视器输出连接成功信息 
    Serial.println(""); 
    Serial.print("ESP8266 Connected to ");
    Serial.println(WiFi.SSID());              // WiFi名称
    Serial.print("IP address:\t");
    Serial.println(WiFi.localIP());           // IP
    tft.fillScreen(TFT_WHITE);
    tft.pushImage(14, 65, 100, 20, ConnectWifi[5]);
    tft.setCursor(20, 30, 1);                //设置文字开始坐标(20,30)及1号字体
    tft.setTextSize(1);
    drawSentences(22,95,"正在初始化",TFT_GREEN);
    tft.println("WiFi Connected!");
    
    delay(500);
}

void show_main(uint16_t fg, uint16_t bg, int currentHour, int currentMinute,int currentMonth,int currentMonthDay,int currentWeekDay){
//tft.fillRect(10, 55,  64, 64, bg);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);             //使图片颜色由RGB->BGR
  //delay(100);
  tft.pushImage(55, 96,  30, 30, temIcon);//温度贴图
  tft.drawFastHLine(4, 25, 120, tft.alphaBlend(0, TFT_RED,  fg));//绘制线  半透明颜色0-255
  tft.drawFastHLine(4, 95, 120, tft.alphaBlend(0, TFT_RED,  fg));//绘制线  半透明颜色0-255
  //tft.drawFastHLine(10, 53, 108, tft.alphaBlend(0, bg,  fg));
  /*时间显示*/
  if(currentHour-10 < 0)
  {showtext(72,7,1,2,TFT_YELLOW,bg," " + (String)currentMonth+"/"+(String)currentMonthDay);}
  else
  {showtext(72,7,1,2,TFT_YELLOW,bg,(String)currentMonth+"/"+(String)currentMonthDay);}//date
  if(currentHour-10 < 0)
  {showtext(70,35,1,3,TFT_GREENYELLOW,bg,"0"+(String)currentHour+":\n");}
  else
  {showtext(70,35,1,3,TFT_GREENYELLOW,bg,(String)currentHour+":\n");}//hour
  if((currentMinute-10)<0)
  {showtext(85,65,1,3,TFT_MAGENTA,bg,"0"+(String)currentMinute);}
  else
  {showtext(85,65,1,3,TFT_MAGENTA,bg,(String)currentMinute);}//minute
  drawSentences(5, 103, weekDays[currentWeekDay], TFT_YELLOW);//weekday

  /*天气显示*/
  tft.pushImage(0, 30,  64, 64, weather[ph]);
  drawSentences(45, 103, now_wea, TFT_YELLOW);//天气
  drawSentences(5, 5, "丽水", TFT_GOLD);//地名
  int temp_color;
  if (atoi(now_temperature.c_str()) <=5 ){
    temp_color=TFT_BLUE;
  }else if(atoi(now_temperature.c_str()) <=15 ){
    temp_color=TFT_SKYBLUE;
  }else if(atoi(now_temperature.c_str()) <=34 ){
    temp_color=TFT_GREEN;
  }else{
    temp_color=TFT_RED;
  }
  if(atoi(now_temperature.c_str())>=10)
  showtext(80,105,1,2,temp_color,bg,now_temperature);//温度
  else
  showtext(80,105,1,2,temp_color,bg," " + now_temperature);//温度
  showtext(105,100,1,1,temp_color,bg,".\n");
  showtext(112,108,1,1,temp_color,bg,"C\n");
}



void setup() {
  Serial.begin(9600);  
  tft.init();                         //初始化显示寄存器
  tft.fillScreen(TFT_WHITE);          //屏幕颜色
  tft.setTextColor(TFT_BLACK);        //设置字体颜色黑色
  tft.setCursor(15, 30, 1);           //设置文字开始坐标(15,30)及1号字体
  tft.setTextSize(1);  
  tft.println("Connecting Wifi...");//准备联网
  tft.setSwapBytes(true);             //使图片颜色由RGB->BGR
  for (int j = 0; j < 5; j++)
  {
      tft.pushImage(14, 65, 100, 20, ConnectWifi[j]);   //调用图片数据
      delay(400);  
  }     
  wifi_connect();//联网
  get_weather();//获取天气
  timeClient.begin();//校时
  timeClient.setTimeOffset(28800);//偏移东八区
  //weather_t.attach(600,get_weather);
}
 
void loop() {
  timeClient.update();
  /*转换时间*/
  unsigned long epochTime = timeClient.getEpochTime();
  Serial.print("Epoch Time: ");
  Serial.println(epochTime);
  //打印时间
  int currentHour = timeClient.getHours();//hour
  Serial.print("Hour: ");
  Serial.println(currentHour);
  int currentMinute = timeClient.getMinutes();//minute
  Serial.print("Minutes: ");
  Serial.println(currentMinute);
  int currentSecond = timeClient.getSeconds();//second
  Serial.print("Second: ");
  Serial.println(currentSecond);
  int currentWeekDay = timeClient.getDay();//weekday
  // String weekDay = weekDays[currentWeekDay];//转换
  // char week[weekDay.length() + 1];//转换
  // weekDay.toCharArray(week,weekDay.length() + 1);//转换
  Serial.print("Week Day: ");
  Serial.println(currentWeekDay);
  //将epochTime换算成年月日
  struct tm *ptm = gmtime ((time_t *)&epochTime);
  int currentMonthDay = ptm->tm_mday;//monthday
  Serial.print("Month day: ");
  Serial.println(currentMonthDay);
  int currentMonth = ptm->tm_mon+1;//month
  Serial.print("Month: ");
  Serial.println(currentMonth);
  delay(1000);
  tick++;
  //四分钟更新一次天气
  if (tick==241){
    get_weather();
    tick=1;
  }
  flag++;
  if (flag==3&&60-currentSecond>5){show_main(TFT_WHITE, TFT_BLACK, currentHour, currentMinute, currentMonth, currentMonthDay,currentWeekDay);flag=0;}
  if (currentSecond == 0)
  show_main(TFT_WHITE, TFT_BLACK, currentHour, currentMinute, currentMonth, currentMonthDay,currentWeekDay); // 显示时间界面
  
}