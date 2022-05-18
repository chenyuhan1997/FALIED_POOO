/**********************************************************************
//本程序使用LVGL来显示网络时间，
增加启动界面IP地址显示，调整为自己对应的硬件1.3寸IPS
LCD   开发板 
GND---->GND
VCC---->3V3
SCL---->D18
SDA---->D23
RES---->D21
DC----->D4
BLK---->3V3

按键
GND---->GND
KEY---->D27
**********************************************************************/
#include "amain.h"

#define  VERSION   "V101"


void Pra_Read(void)                             //读取EEPROM数据
{
  uint16_t i;
  uint8_t *p = (uint8_t*)(&Set_Pra);            //配置结构体指针

  EEPROM.begin(200);                            //申请操作字节数据
  for(i = 0; i < sizeof(Config_Set); i ++){
    p[i] = EEPROM.read(i);                      //读数据
  }
  if (Set_Pra.store != 0x55AA) {                //没有写入过数据的话
    memset(Set_Pra.ssid, '\0', sizeof(Set_Pra.ssid));
    memset(Set_Pra.pass, '\0', sizeof(Set_Pra.pass));
    memset(Set_Pra.city, '\0', sizeof(Set_Pra.city));
    Read_Pra_Flag = false;
    memcpy(Set_Pra.city, cityCode.c_str(), sizeof(cityCode));
  }
  else{
    Read_Pra_Flag = true;
  }
  
  //Serial.printf("ssid: %s\n", Set_Pra.ssid);
  //Serial.printf("pass: %s\n", Set_Pra.pass);
  //Serial.printf("city: %s\n", Set_Pra.city);
}

void Pra_Write(void)                            //读取EEPROM数据
{
  uint16_t i;
  uint8_t *p = (uint8_t*)(&Set_Pra);            //配置结构体指针

  Set_Pra.store = 0x55AA;                       //置标志位
  EEPROM.begin(200);                            //申请操作字节数据
  for(i = 0; i < sizeof(Config_Set); i ++){
     EEPROM.write(i, p[i]);
  }
  EEPROM.commit();                              //保存数据
  
  Serial.printf("ssid: %s\n", Set_Pra.ssid);
  Serial.printf("pass: %s\n", Set_Pra.pass);
  Serial.printf("city: %s\n", Set_Pra.city);
}

//---------------------------WEB 设置开始-----------------------------------
void handleMain()                               //打开主页面，打开设置都是调用这个函数
{
  int web_cc;

  if (server.hasArg("web_ccode")){              //设置参数参在的话，判断处理
    web_cc = server.arg("web_ccode").toInt();
    if ((web_cc >= 101000000) && (web_cc <= 102000000)){   //设置范围判断
      uint8_t len = server.arg("web_ccode").length() + 1;
      char buf[15];
      server.arg("web_ccode").toCharArray(buf, len);//字符串转到数组
          
      memset(Set_Pra.city, '\0', sizeof(Set_Pra.city));
      memcpy(Set_Pra.city, buf, len);
      Pra_Write();                              //保存数据.c_str()

      String message = "POST form was:\r\n";
      for (uint8_t i = 0; i < (server.args() - 1); i++){  //打印设置信息
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
      }
      delay(500);
      server.send(200, "text/plain", message);
      server.handleClient();                    //WEB处理函数
      delay(500);
      ESP.restart();                            //保存数据后立刻重启
    }
  }

  //网页界面代码段
  String content = "<html><style>html,body{ background: #1aceff; color: #fff; font-size: 10px;}</style>";
        content += "<body><form action='/' method='POST'><br><div>Weather Web Config</div><br>";
        content += "City Code:<br><input type='text' name='web_ccode' placeholder='city code'><br>";
        content += "<br><div><input type='submit' name='Save' value='Save'></form></div>";
        content += "</body></html>";
  server.send(200, "text/html", content);
}

void handleNotFound()                           //网页不存在打印
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  
  for (uint8_t i = 0; i < server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  
  server.send(404, "text/plain", message);
}

void Web_Sever_Init()                           //Web服务初始化
{
  uint32_t counttime = millis();                //记录创建mDNS的时间
  while (!MDNS.begin(host1)){
    if (millis() - counttime > 30000){
      ESP.restart();                            //判断超过30秒钟就重启设备
    }
  }
  MDNS.addService("http", "tcp", 80);           //将服务器添加到mDNS
  
  Serial.println("MDNS started");
  server.on("/", handleMain);                   //请求打开网页
  server.onNotFound(handleNotFound);
  server.begin();                               //开启HTTP服务
  Serial.println("HTTP server started");
  Serial.printf("Open \"http://%s.local\" in your browser to set\r\n", host1);
  Serial.print("IP： ");
  Serial.println(WiFi.localIP());
}

void Web_Server_Deal()                          //Web网页设置函数
{
  //MDNS.update();
  server.handleClient();
}
//---------------------------WEB 设置结束-----------------------------------

void WiFiManager_Start (void)                   //WiFiManager配置启动
{
  WiFi.mode(WIFI_STA);                          //WIFI模式
  
  delay(1000);
  wm.resetSettings();                           //复位为默认设置

  WiFiManagerParameter  custom_cc("CityCode","CityCode",Set_Pra.city,9);
  WiFiManagerParameter  p_lineBreak_notext("<p></p>");

  wm.addParameter(&p_lineBreak_notext);         //添加参数
  wm.addParameter(&custom_cc);
  wm.addParameter(&p_lineBreak_notext);

  wm.setSaveParamsCallback(saveParamCallback);  //参数设置完成回调函数
  
  std::vector<const char *> menu = {"wifi","restart"};
  wm.setMenu(menu);
  wm.setClass("invert");
  wm.setMinimumSignalQuality(20);               //过滤WIFI信号强度

  String apName = ("天气" + (String)(uint32_t)ESP.getEfuseMac()); //设置WIFI名称
  const char *softAPName = apName.c_str();
  bool res = wm.autoConnect(softAPName);        //设置配网热点
  while(!res);
}

String getParam(String name)                    //读取设置的数据
{
  String value;
  if(wm.server->hasArg(name)) {
    value = wm.server->arg(name);
  }
  return value;
}

void saveParamCallback()                        //参数设置完成回调函数
{
  //Serial.println("[CALLBACK] saveParamCallback fired");
  //Serial.println("PARAM CityCode = " + getParam("CityCode"));

  uint8_t len = getParam("CityCode").length() + 1;
  char buf[15];
  getParam("CityCode").toCharArray(buf, len);   //字符串转到数组
      
  memset(Set_Pra.city, '\0', sizeof(Set_Pra.city));
  memcpy(Set_Pra.city, buf, len);
  Pra_Write();                                  //保存数据.c_str()
}

void Led_On(void)                               //模块上二极管亮
{
   digitalWrite(Led, HIGH);                     //高为开，低为关
   Led_State = true;
}

void Led_Off(void)                              //模块上二极管灭
{
   digitalWrite(Led, LOW);                      //高为开，低为关
   Led_State = false;
}

void digitalClockDisplay()                      //打印时间
{
  Time.Year = year();                           //得到年份
  Time.Month = month();                         //得到月份
  Time.Date = day();                            //得到日期
  Time.Hour = hour();                           //小时
  Time.Minute = minute();                       //分钟        
  Time.Second = second();                       //秒钟
  Time.Week = weekday();                        //周

  if (Time.Second == 0){                        //一分钟打印一次时间
    Serial.printf("%d/%d/%d %d:%d:%d Weekday:%d\n", Time.Year, Time.Month, Time.Date, Time.Hour, Time.Minute, Time.Second, weekday());
  }

  PowerOn_Flag = 1;

  String currentTime = "#0000ff ";
  if (Time.Hour < 10)
    currentTime += 0;
  currentTime += Time.Hour;
  currentTime += ":";
  if (Time.Minute < 10)
    currentTime += 0;
  currentTime += Time.Minute;
  currentTime += ":";
  if (Time.Second < 10)
    currentTime += 0;
  currentTime += Time.Second;
  currentTime += " #";
  
  /*String currentDay = "";
  currentDay += Time.Year;
  currentDay += "/";
  if (Time.Month < 10)
    currentDay += 0;
  currentDay += Time.Month;
  currentDay += "/";
  if (Time.Date < 10)
    currentDay += 0;
  currentDay += Time.Date;*/

  String week1 = "";
  switch(Time.Week){
    case 1:
      week1 = "日";
      break;
    case 2:
      week1 = "一";
      break;
    case 3:
      week1 = "二";
      break;
    case 4:
      week1 = "三";
      break;
    case 5:
      week1 = "四";
      break;
    case 6:
      week1 = "五";
      break;
    case 7:
      week1 = "六";
      break;
    default:
      break;
  }
  if (Page_Id == 0){                             //天气页面更新数据
    lv_label_set_text(labelTime, currentTime.c_str());//设置内容
    //lv_label_set_text_fmt(labelTime, "#0000ff %02d:%2d:%2d #", Time.Hour, Time.Minute, Time.Second);//设置内容
    lv_label_set_text_fmt(labelDay, "#0000ff %4d年%d月%d日  星期%s #", Time.Year, Time.Month, Time.Date, week1.c_str());//设置内容
  }
}
  
void printDigits(int digits)                    //打印时间数据
{
  Serial.print(":");
  if (digits < 10)                              //打印两位数字
    Serial.print('0');
  Serial.print(digits);
}

time_t getNtpTime()                             //获取NTP时间
{
  bool Time_Recv_Flag = false;
  IPAddress ntpServerIP;                        //NTP服务器的IP地址
  
  while (Udp.parsePacket() > 0) ;               //之前的数据没有处理的话一直等待 discard any previously received packets
  WiFi.hostByName(ntpServerName, ntpServerIP);  //从网站名获取IP地址
  if (PowerOn_Flag == 0)                        //第一次才打印
  {
    Serial.println("Transmit NTP Request");
    Serial.print(ntpServerName);
    Serial.print(": ");
    Serial.println(ntpServerIP);
  }

  while (Time_Recv_Flag == false)               //如果是上电第一次获取数据的话，要一直等待，直到获取到数据，不是第一次的话，没获取到数据，直接返回
  {
    sendNTPpacket(ntpServerIP);                 //发送数据包
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500){
      int size = Udp.parsePacket();             //接收数据
      if (size >= NTP_PACKET_SIZE){
        Serial.println("Receive NTP Response");
        Udp.read(packetBuffer, NTP_PACKET_SIZE);//从缓冲区读取数据
        
        unsigned long secsSince1900;
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        if (PowerOn_Flag == 0){                 //第一次收到数据，清屏，画表盘
        }
        Time_Recv_Flag = true;
        return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      }
    }
    if (PowerOn_Flag == 1){                     //不是第一次
      return 0;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;                                     //没获取到数据的话返回0
}
  
void sendNTPpacket(IPAddress &address)          //发送数据包到NTP服务器
{
  memset(packetBuffer, 0, NTP_PACKET_SIZE);     //缓冲区清零

  packetBuffer[0] = 0b11100011;                 // LI, Version, Mode   填充缓冲区数据
  packetBuffer[1] = 0;                          // Stratum, or type of clock
  packetBuffer[2] = 6;                          // Polling Interval
  packetBuffer[3] = 0xEC;                       // Peer Clock Precision
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  Udp.beginPacket(address, 123);                //NTP服务器端口123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);     //发送udp数据
  Udp.endPacket();                              //发送结束
  
  if (PowerOn_Flag == 0){
    Serial.println("Send Ntp data...");
  }
}

void my_print(lv_log_level_t level, const char* file, uint32_t line, const char* fun, const char* dsc)
{
    Serial.printf("%s@%d %s->%s\r\n", file, line, fun, dsc);
    Serial.flush();
}

//area 参数定义了本次刷新的位置和区域，color_p 参数定义了本次刷新的所需要的缓冲区
void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

void Connet_WIFI_Dis (void)                     //创建联网界面 
{
  img1 = lv_img_create(scr, NULL);              //显示图像
  lv_img_set_src(img1, &bing);                  //图像路径
  lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, -50);//设置控件的对齐方式-相对坐标

  lv_style_init(&style1);                       //样式，用于进度条背景
  lv_style_set_border_color(&style1, LV_STATE_DEFAULT, LV_COLOR_BLUE); //指定边框的颜色
  lv_style_set_border_width(&style1, LV_STATE_DEFAULT, 2); //设置边框的宽度
  lv_style_set_border_opa(&style1, LV_STATE_DEFAULT, LV_OPA_50); //指定边框的不透明度
  lv_style_init(&style2);                       //样式，用于进度条指示器
  lv_style_set_bg_color(&style2, LV_STATE_DEFAULT, LV_COLOR_BLUE);

  bar1 = lv_bar_create(scr, NULL);              //创建进度条，联网指示用
  lv_obj_set_size(bar1, 220, 15);               //进度条尺寸，长，宽
  lv_bar_set_anim_time(bar1, 100);              //动画的时间ms
  lv_bar_set_value(bar1, bar1_value, 1);        //进度条显示值，最后参数1的话使用动画
  lv_obj_add_style(bar1, LV_OBJ_PART_MAIN, &style1); //进度条背景
  lv_obj_add_style(bar1, LV_BAR_PART_INDIC, &style2);//进度条指示器
  lv_obj_align(bar1, NULL, LV_ALIGN_CENTER, 0, 30);//设置控件的对齐方式-相对坐标
  
  label1 = lv_label_create(scr,NULL);           //创建label控件
  lv_obj_add_style(label1, LV_LABEL_PART_MAIN, &style_cn);
  //lv_obj_set_pos(label1, 80, 130);            //设置控件的坐标
  lv_label_set_text(label1, "连接路由器");        //设置文字
  lv_label_set_align(label1, LV_LABEL_ALIGN_CENTER);//居中显示
  lv_obj_align(label1, NULL, LV_ALIGN_CENTER, 0, 55);//设置控件的对齐方式-相对坐标

  label2 = lv_label_create(scr,NULL);           //创建label控件
  lv_obj_add_style(label2, LV_LABEL_PART_MAIN, &style_cn);
  //lv_label_set_long_mode(label2, LV_LABEL_LONG_SROLL_CIRC); //太长的话，滚动显示
  lv_label_set_long_mode(label2, LV_LABEL_LONG_BREAK); //太长的话，分行显示
  lv_label_set_recolor(label2, true);           //重新着色
  lv_obj_set_width(label2, 220);                //设置大小
  lv_label_set_text(label2, "");                //设置文字
  lv_label_set_align(label2, LV_LABEL_ALIGN_CENTER);//居中显示
  lv_obj_align(label2, NULL, LV_ALIGN_CENTER, 0, 80);//设置控件的对齐方式-相对坐标
}

void Wifi_Connect_Deal (void)                   //联网处理
{
  Serial.print("Connet WIFI: ");
  Serial.println(Set_Pra.ssid);
  WiFi.begin(Set_Pra.ssid, Set_Pra.pass);

  Connet_WIFI_Dis ();                           //创建联网界面
  while (WiFi.status() != WL_CONNECTED){        //连接路由器
    bar1_value += 5;
    lv_bar_set_value(bar1, bar1_value, 1);      //进度条显示值，最后参数1的话使用动画
    lv_task_handler();                          //开启lv调度
    delay(1000);
    if ((bar1_value > 100) || (Read_Pra_Flag == false)){//进度走到头还没联上网的话，启动网页配网
      lv_bar_set_value(bar1, 0, 1);             //进度条显示值，最后参数1的话使用动画
      lv_label_set_text(label1,"启动网页配网");   //设置文字
      lv_label_set_text(label2,"请用手机连接热点并用浏览器\n打开\"#ff0000 192.168.4.1#\"进行配网");      //设置文字
      lv_task_handler();                        //开启lv调度
      WiFiManager_Start();
      break;
    }
  }

  if(WiFi.status() == WL_CONNECTED){            //连上路由器的话
    strcpy(Set_Pra.ssid,WiFi.SSID().c_str());   //名称复制
    strcpy(Set_Pra.pass,WiFi.psk().c_str());    //密码复制
    lv_label_set_text(label1, "连接成功");        //设置文字
    lv_label_set_text_fmt(label2, "IP: #ff0000 %s #", WiFi.localIP().toString().c_str());//显示IP地址
    lv_task_handler();                          //开启lv调度
    Pra_Write();                                //保存路由器数据
    Web_Sever_Init();                           //开启web服务器初始化
    delay(2000);
  }
  lv_obj_del(label1);                           //删除对象
  lv_obj_del(label2); 
  lv_obj_del(bar1);
  lv_obj_del(img1);

  Serial.println("Starting UDP");               //连接时间服务器
  Udp.begin(localPort);
  Serial.println("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

void Weather_Img_Dis (int num)                  //显示天气图标
{
  if (num == 00){
    lv_img_set_src(wimg1, &d00);                //图像路径
  }
  else if(num == 01){
    lv_img_set_src(wimg1, &d01);                //图像路径
  }
  else if(num == 02){
    lv_img_set_src(wimg1, &d02);                //图像路径
  }
  else if(num == 03){
    lv_img_set_src(wimg1, &d03);                //图像路径
  }
  else if(num == 04){
    lv_img_set_src(wimg1, &d04);                //图像路径
  }
  else if(num == 05){
    lv_img_set_src(wimg1, &d05);                //图像路径
  }
  else if(num == 06){
    lv_img_set_src(wimg1, &d06);                //图像路径
  }
  else if(num == 07||num == 8||num == 21||num == 22){
    lv_img_set_src(wimg1, &d07);                //图像路径
  }
  else if(num == 9||num == 10||num == 23||num == 24){
    lv_img_set_src(wimg1, &d09);                //图像路径
  }
  else if(num == 11||num == 12||num == 25||num == 301){
    lv_img_set_src(wimg1, &d11);                //图像路径
  }
  else if(num == 13){
    lv_img_set_src(wimg1, &d13);                //图像路径
  }
  else if(num == 14||num == 26){
    lv_img_set_src(wimg1, &d14);                //图像路径
  }
  else if(num == 15||num == 27){
    lv_img_set_src(wimg1, &d15);                //图像路径
  }
  else if(num == 16||num == 17||num == 28||num == 302){
    lv_img_set_src(wimg1, &d16);                //图像路径
  }
  else if(num == 18){
    lv_img_set_src(wimg1, &d18);                //图像路径
  }
  else if(num == 19){
    lv_img_set_src(wimg1, &d19);                //图像路径
  }
  else if(num == 20){
    lv_img_set_src(wimg1, &d20);                //图像路径
  }
  else if(num == 29){
    lv_img_set_src(wimg1, &d29);                //图像路径
  }
  else if(num == 30){
    lv_img_set_src(wimg1, &d30);                //图像路径
  }
  else if(num == 31){
    lv_img_set_src(wimg1, &d31);                //图像路径
  }
  else if(num == 53||num == 32||num == 49||num == 54||num == 55||num == 56||num == 57||num == 58){
    lv_img_set_src(wimg1, &d53);                //图像路径
  }
  else{
    //lv_img_set_src(wimg1, &d99);                //图像路径
    //lv_obj_del(img1);
  }
}

void getCityWeater(){                           //获取城市天气
  //String URL = "http://d1.weather.com.cn/dingzhi/" +  String(Set_Pra.city) + ".html?_="+String(now());//新
  String URL = "http://d1.weather.com.cn/weather_index/" + String(Set_Pra.city) + ".html?_="+String(now());//原来
  
  HTTPClient httpClient;                        //创建 HTTPClient 对象
  httpClient.begin(URL);                        //配置请求地址。
  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");
 
  int httpCode = httpClient.GET();              //启动连接并发送HTTP请求
  Serial.println("get weather data");
  Serial.println(URL);
  
  if (httpCode == HTTP_CODE_OK) {               //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
    String str = httpClient.getString();
    //Serial.println(str);
    
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");
    String jsonCityDZ = str.substring(indexStart+13,indexEnd);//获取有效数据部分
    //Serial.println(jsonCityDZ);

    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    String jsonDataSK = str.substring(indexStart+8,indexEnd);//获取有效数据部分
    //Serial.println(jsonDataSK);

    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    String jsonFC = str.substring(indexStart+5,indexEnd);//获取有效数据部分
    //Serial.println(jsonFC);
    
    weaterData(&jsonCityDZ, &jsonDataSK, &jsonFC);//处理天气数据并显示
    Serial.println("got weather ok");
  } 
  else {
    Serial.println("err code：");
    Serial.print(httpCode);
  }
 
  httpClient.end();                             //关闭与服务器连接
}

void weaterData (String *cityDZ, String *dataSK, String *dataFC)//天气信息写到屏幕上
{
  DynamicJsonDocument doc(1024);                //解析第一段JSON
  deserializeJson(doc, *dataSK);
  JsonObject sk = doc.as<JsonObject>();

  String aqiTxt = "优";
  int pm25V = sk["aqi"];                        //PM2.5空气指数
  if(pm25V>200){
    aqiTxt = "重度";
  }else if(pm25V>150){
    aqiTxt = "中度";
  }else if(pm25V>100){
    aqiTxt = "轻度";
  }else if(pm25V>50){
    aqiTxt = "良";
  }
  Weather_Text[4] = "#ff0000 \uF2C9温度 #" + sk["temp"].as<String>() + "℃" + "\n";//温度
  Weather_Text[5] = "#00ff00 \uF043湿度 #" + sk["SD"].as<String>() + "\n";//湿度
  Weather_Text[6] = "pm25 " + sk["aqi_pm25"].as<String>() + "\n";//PM25
   
  //Weather_Text[0] = "城市: " + sk["cityname"].as<String>() + "\n";
  //Weather_Text[2] = "空气质量: " + aqiTxt + "\n";
  //Weather_Text[3] = "风向: " + sk["WD"].as<String>() + sk["WS"].as<String>() + "\n";
  Weather_Text[0] = sk["cityname"].as<String>() + "   ";     //城市
  Weather_Text[2] = "空气质量: " + aqiTxt + "\n";             //空气质量
  Weather_Text[3] = sk["WD"].as<String>() + sk["WS"].as<String>() + "\n";//风向
  
  //显示天气图标
  Weather_Img_Dis(atoi((sk["weathercode"].as<String>()).substring(1,3).c_str()));
  
  deserializeJson(doc, *cityDZ);                //解析第二段JSON
  JsonObject dz = doc.as<JsonObject>();
  //Weather_Text[1] = "天气: " + dz["weather"].as<String>() + "\n";
  Weather_Text[1] = dz["weather"].as<String>() + "\n";
  
  deserializeJson(doc, *dataFC);                //解析第三段JSON
  JsonObject fc = doc.as<JsonObject>();
  
  Data_Dis = Weather_Text[0] + Weather_Text[1] + Weather_Text[3];
  //Data_Dis += "温度: " + fc["fd"].as<String>() + " ~ " + fc["fc"].as<String>()+ "℃\n";
  Data_Dis += "温度: " + fc["fd"].as<String>() + " ~ " + fc["fc"].as<String>()+ "℃\n";
  Data_Dis += Weather_Text[2];
 
  lv_label_set_text(label1, (Data_Dis).c_str());//显示数据
  lv_label_set_text(label2, "");
  lv_label_set_text(label2, (Weather_Text[4] + Weather_Text[5] + Weather_Text[6]).c_str()); 
}

void Bing_Animing (uint8_t num)                 //动画显示
{
  switch (num){
    case 1:
      lv_img_set_src(img2, &bing01);
    break;
    case 2:
      lv_img_set_src(img2, &bing02);
    break;
    case 3:
      lv_img_set_src(img2, &bing03);
    break;
    case 4:
      lv_img_set_src(img2, &bing04);
    break;
    case 5:
      lv_img_set_src(img2, &bing05);
    break;
    case 6:
      lv_img_set_src(img2, &bing06);
    break;
    default:
    break;
  }
}

void Lvgl_Dis_Init (void)                       //LVGL初始化
{
  lv_init();                                    //初始化，这里面会初始化lv库的链表、内存、数据结构管理等系统内部的东西
  tft.begin();
  tft.setRotation(0);
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);//初始化缓存区

  lv_disp_drv_t disp_drv;                       //定义显示设备
  lv_disp_drv_init(&disp_drv);                  //初始化显示设备
  disp_drv.hor_res = 240;                       //屏大小
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = disp_flush;               //注册刷新函数
  disp_drv.buffer = &disp_buf;                  //设备得到的显示缓冲地址
  lv_disp_drv_register(&disp_drv);

  scr = lv_disp_get_scr_act(NULL);              //获取当前屏幕
  
  lv_style_init(&style_cn);                     //字体样式初始化
  lv_style_set_text_font(&style_cn, LV_STATE_DEFAULT, &myFont1);//显示汉字
}

void Weather_Page_Create (lv_obj_t *parent)     //天气界面控件初始化
{
  //天气图标控件初始化
  wimg1 = lv_img_create(parent, NULL);          //显示天气图像
  //lv_img_set_zoom(wimg1, 320);                //256正常,<256缩小,>256放大,100*100
  lv_img_set_zoom(wimg1, 288);                  //256正常,<256缩小,>256放大,90*90
  lv_obj_set_pos(wimg1, 5, 0);                  //设置的位置 

  //动画控件初始化
  img2 = lv_img_create(parent, NULL);           //显示动画
  //lv_img_set_zoom(img2, 96);                  //256正常,<256缩小,>256放大,90*90原图240*240
  lv_img_set_zoom(img2, 192);                   //256正常,<256缩小,>256放大,90*90原图120*120
  //lv_obj_set_auto_realign(img2, true);
  //lv_obj_set_pos(img2, 0, 126);               //设置的位置 
  lv_obj_align(img2, parent, LV_ALIGN_IN_BOTTOM_RIGHT, -30, -60);//对齐

  //时间显示控件初始化
  lv_style_init(&style_led);                    //字体样式初始化，用于时间显示
  //lv_style_set_text_font(&style_led, LV_STATE_DEFAULT, &lv_font_montserrat_48);
  lv_style_set_text_font(&style_led, LV_STATE_DEFAULT, &myFontLed);

  labelTime = lv_label_create(parent, NULL);    //左右居中显示时间
  lv_obj_add_style(labelTime, LV_LABEL_PART_MAIN, &style_led);//添加字体
  lv_obj_set_pos(labelTime, 0, 85);             //设置的位置 
  lv_label_set_recolor(labelTime, true);        //重新着色
  //lv_label_set_long_mode(labelTime, LV_LABEL_LONG_SROLL); //保持大小并循环滚动标签
  lv_label_set_align(labelTime, LV_LABEL_ALIGN_CENTER); //居中显示
  lv_obj_set_width(labelTime, 240);             //标签宽度

  //日期显示控件初始化
  labelDay = lv_label_create(parent, NULL);     //移动显示提起
  lv_obj_add_style(labelDay, LV_LABEL_PART_MAIN, &style_cn);//添加字体
  lv_obj_set_pos(labelDay, 0, 220);             //设置的位置 
  lv_label_set_recolor(labelDay, true);         //重新着色
  lv_label_set_long_mode(labelDay, LV_LABEL_LONG_SROLL_CIRC); //保持大小并循环滚动标签
  lv_label_set_align(labelDay, LV_LABEL_ALIGN_CENTER); //居中显示
  lv_obj_set_width(labelDay, 140);              //标签宽度

  //天气信息控件初始化
  label1 = lv_label_create(parent, NULL);       //显示天气信息
  lv_obj_add_style(label1, LV_LABEL_PART_MAIN, &style_cn);//添加汉字字体
  lv_obj_set_pos(label1, 100, 0);               //设置的位置 
  //lv_label_set_long_mode(label1, LV_LABEL_LONG_SROLL); 
  lv_label_set_align(label1, LV_LABEL_ALIGN_CENTER);//居中对齐
  lv_obj_set_width(label1, 140);                //宽度
  //lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 20); 

  label2 = lv_label_create(parent, NULL);       //显示天气信息
  lv_obj_add_style(label2, LV_LABEL_PART_MAIN, &style_cn);//添加汉字字体
  lv_obj_set_pos(label2, 10, 150);              //设置的位置 
  lv_label_set_recolor(label2, true);           //重新着色
  lv_label_set_align(label2, LV_LABEL_ALIGN_LEFT);//左对齐
  lv_obj_set_width(label2, 150);                //宽度
  //lv_obj_align(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 20); 
}

void Time_Page_Create (lv_obj_t *parent)        //时钟界面控件初始化
{
  //lv_obj_set_size(parent, LV_HOR_RES_MAX, LV_VER_RES_MAX);//设置大小和位置
  //lv_obj_set_pos(parent,  0, 0);
  
  Watch_Img = lv_img_create(parent,NULL);       //背景图片
  lv_img_set_src(Watch_Img, &cwatch_bg);        //图片地址
  //lv_img_set_zoom(Watch_Img, 307);              //256正常,<256缩小,>256放大 240*240
  lv_obj_set_auto_realign(Watch_Img, true);
  lv_obj_align(Watch_Img, parent, LV_ALIGN_CENTER, 0, 0);//对齐
  
  Hour_Img = lv_img_create(parent,NULL);
  lv_img_set_src(Hour_Img, &chour);
  lv_img_set_zoom(Hour_Img, 307);               //256正常,<256缩小,>256放大
  lv_obj_align(Hour_Img, Watch_Img, LV_ALIGN_CENTER, 0, 0);
  uint16_t h = Time.Hour * 300 + Time.Minute / 12 % 12 * 60;
  lv_img_set_angle(Hour_Img, h);                //设置角度
  
  Minute_Img = lv_img_create(parent,NULL);
  lv_img_set_src(Minute_Img, &cminute);
  lv_img_set_zoom(Minute_Img, 307);             //256正常,<256缩小,>256放大
  lv_obj_align(Minute_Img, Watch_Img,LV_ALIGN_CENTER, 0, 0);
  lv_img_set_angle(Minute_Img, Time.Minute * 60);//设置角度
  
  Second_Img = lv_img_create(parent,NULL);
  lv_img_set_src(Second_Img, &csecond);
  lv_img_set_zoom(Second_Img, 307);             //256正常,<256缩小,>256放大
  lv_obj_align(Second_Img, Watch_Img, LV_ALIGN_CENTER, 0, 0);
  lv_img_set_angle(Second_Img, Time.Second * 60);//设置角度
  
  //lv_task_create(update_time, 1000, LV_TASK_PRIO_LOW, NULL);
}

void Calendar_Page_Create (lv_obj_t *parent)    //日历界面控件初始化
{
  today_date.year = Time.Year;                  //日期
  today_date.month = Time.Month;
  today_date.day = Time.Date;

  lv_style_init(&style1);                       //样式，用于日期
  //lv_style_reset(&style1);                    //重置样式，复位之前所有数据
  lv_style_set_bg_color(&style1, LV_STATE_FOCUSED, LV_COLOR_RED); //指定背景的颜色
  lv_style_set_radius(&style1, LV_STATE_FOCUSED, 15);
  
  lv_style_init(&style2);                       //样式，用于头部，年月，去掉了左右箭头
  lv_style_set_text_font(&style2, LV_STATE_DEFAULT, &myFont1);//显示汉字
  
  lv_style_init(&style3);                       //样式，用于周样式
  lv_style_set_bg_color(&style3, LV_STATE_FOCUSED, LV_COLOR_RED); //指定背景的颜色
  lv_style_set_text_font(&style3, LV_STATE_DEFAULT, &myFont1);//显示汉字

  calendar1 = lv_calendar_create(parent, NULL); //创建日历对象  
  lv_obj_set_size(calendar1, 240, 240);         //设置大小
  lv_calendar_set_day_names(calendar1, day_names);//设置星期名称
  lv_calendar_set_month_names(calendar1, month_names);//设置月份名称
  //lv_calendar_set_today_date(calendar1, &today_date);//设置当前日期
  //lv_calendar_set_showed_date(calendar1, &today_date);//设置显示日期
  lv_obj_add_style(calendar1, LV_CALENDAR_PART_DATE, &style1); //日期
  lv_obj_add_style(calendar1, LV_CALENDAR_PART_HEADER, &style2); //头部，年月
  lv_obj_add_style(calendar1, LV_CALENDAR_PART_DAY_NAMES, &style3); //周样式
  lv_obj_align(calendar1, NULL, LV_ALIGN_CENTER, 0, 0);//居中对齐
}

void Page_Turn_Deal (uint8_t num)             //显示页面切换，num为要显示的页面好0-2
{
  if (num == 0){                              //先删除上一个页面控件，再建立本页面控件
    lv_obj_del(calendar1);                    //删除对象
    LastTime2 = 0;
    Weather_Page_Create (scr);                //天气页面控件初始化
  }
  else if (num == 1){                         //先删除上一个页面控件，再建立本页面控件
    lv_obj_del(img2);                         //删除对象
    lv_obj_del(wimg1);
    lv_obj_del(label1);
    lv_obj_del(label2); 
    lv_obj_del(labelTime);
    lv_obj_del(labelDay);
    Time_Page_Create (scr);                   //时钟页面控件初始化
  }
  else if (num == 2){                         //先删除上一个页面控件，再建立本页面控件
    lv_obj_del(Watch_Img);                    //删除对象
    lv_obj_del(Hour_Img);
    lv_obj_del(Minute_Img);
    lv_obj_del(Second_Img);
    today_date.year = Time.Year;              //日历控件处理
    today_date.month = Time.Month;
    today_date.day = Time.Date;
    Calendar_Page_Create (scr);               //日历页面控件初始化
    lv_calendar_set_today_date(calendar1, &today_date);//设置当前日期
    lv_calendar_set_showed_date(calendar1, &today_date);//设置显示日期
  }
}

void Key_Num_Init (void)
{
  Key_Down_Time = 0;
  Key_Buff[0] = 0xFF;
  Key_Buff[1] = 0xFF;
}

void Key_Scan (void)                            //按键扫描
{
  uint8_t buff[2];
  uint8_t data_temp = 0xFF;
  uint32_t keytimetmp_down = 0;

  if (digitalRead(KEY1) == LOW){                //按键按下了
    data_temp = 0xFE;
  }
  else{
    data_temp = 0xFF;
  }
  
  if(data_temp == 0xFF){                        //此时无键按下
    keytimetmp_down = Key_Down_Time;
    Key_Num_Init ();

    if(keytimetmp_down < 1){                    //已经处理过的不理睬
      return;
    }

    Page_Id ++;                                 //页签ID增加处理
    if (Page_Id >= 3){
      Page_Id = 0;
    }
    Page_Turn_Deal (Page_Id);                   //显示页面切换
    return;                                     //其它的不理睬  
  }

  //*****************************************下面为有键按下时处理*****************************************
  Key_Buff[0] = Key_Buff[1];
  Key_Buff[1] = data_temp;
  if (Key_Buff[0] == Key_Buff[1]){              //确认有键按下
    Key_Down_Time ++;                           //按键次数
  }
  else {
    Key_Down_Time = 0;                          //按键次数=0
  }
}

void setup()
{
  Serial.begin(115200);                         //初始化串口
  Serial.println();                             //打印回车换行

  pinMode(Led, OUTPUT);                         //初始化IO
  Led_Off();
  pinMode(KEY1, INPUT_PULLUP);                  //初始化IO
  
  Pra_Read();                                   //读取EEPROM数据
  Lvgl_Dis_Init ();                             //LVGL初始化
  Wifi_Connect_Deal ();                         //联网处理

  Weather_Page_Create (scr);                    //天气页面控件初始化
}

void loop()
{
  Web_Server_Deal();                            //网页处理
  lv_task_handler();                            //开启lv调度

  if (timeStatus() != timeNotSet){              //已经获取到数据的话
    if (now() != prevDisplay){                  //如果本次数据和上次不一样的话，刷新
      prevDisplay = now();
      digitalClockDisplay();

      if (Page_Id == 1){                        //时钟页面
        uint16_t h = Time.Hour * 300 + Time.Minute / 12 % 12 * 60;//时间控件处理
        lv_img_set_angle(Hour_Img, h);
        lv_img_set_angle(Minute_Img, Time.Minute * 6 * 10);
        lv_img_set_angle(Second_Img, Time.Second * 6 * 10);
      }
      else if (Page_Id == 2){                   //日历页面
        today_date.year = Time.Year;            //日历控件处理
        today_date.month = Time.Month;
        today_date.day = Time.Date;
        lv_calendar_set_today_date(calendar1, &today_date);//设置当前日期
        lv_calendar_set_showed_date(calendar1, &today_date);//设置显示日期
      }
    }
  }

  now2 = millis();                              //定时，固定一段时间获取一次天气数据
  if ((now2 - LastTime2 > 600000) || (LastTime2 == 0)){ //ms
    LastTime2 = now2;
    getCityWeater();                            //获取天气数据
  }

  now1 = millis();                              //定时，固定一段时间获取一次天气数据
  if ((now1 - LastTime1 > 220) && (Page_Id == 0)){//ms
    LastTime1 = now1;
    img_num ++;
    if (img_num > 6){
      img_num = 1;
    }
    Bing_Animing (img_num);                     //动画显示
  }

  now3 = millis();                              //定时扫描按键
  if (now3 - LastTime3 > 20){                   //20MS扫描一次按键
    LastTime3 = now3;
    Key_Scan();
  }
}
