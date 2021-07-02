#include <WiFi.h>
#include "PubSubClient.h"//默认，加载MQTT库文件
#include "DFRobot_DHT11.h"

const char* ssid = "602iot";                  //修改，修改为你的路由的WIFI名字
const char* password = "18wulian";           //修改为你的WIFI密码
const char* mqtt_server = "bemfa.com";       //默认，MQTT服务器地址
const int mqtt_server_port = 9501;          //默认，MQTT服务器端口
#define ID_MQTT  "fcaad2e7f597b6bd81942b52d640bdf1"   //mqtt客户端ID，修改为你的开发者密钥
const char*  ledtopic = "led";                       //Led主题名字，可在巴法云控制台自行创建，名称随意
const char*  fantopic = "fan";                       //Led主题名字，可在巴法云控制台自行创建，名称随意
const char* dhttopic = "temp";                 //温湿度主题名字，可在巴法云mqtt控制台创建

//int pinDHT11 = 13;                         //dht11传感器引脚输入
#define DHT11_PIN  13                        //定义DHT11的引脚
#define Sensor_AO 35                         //定义烟雾传感器的A0引脚
#define Sensor_DO 27                         //定义烟雾传感器的D0引脚

int relay = 32;                             //继电器引脚
int B_led = 33;                             //控制的led引脚
long timeval = 3 * 1000;                    //上传的传感器时间间隔，默认3秒
#define Auto 1
#define Maul 0
boolean Mode = Auto;

unsigned int sensorValue = 0;
String ledstatus = "ledoff";//led状态默认off
String fanstatus = "fanoff";//fan状态默认off
String Alarmstatus = "no alarm";
long lastMsg = 0;//时间戳
//SimpleDHT11 dht11(pinDHT11);//dht11初始化
DFRobot_DHT11 DHT;
WiFiClient espClient;
PubSubClient client(espClient);//mqtt初始化

//灯光函数及引脚定义
void turnOnLed();
void turnOffLed();
//风扇函数及引脚定义
void turnOnFan();
void turnOffFan();

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* ledtopic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(ledtopic);
  Serial.print("] ");
  String Mqtt_Buff = "";
  for (int i = 0; i < length; i++) {
    Mqtt_Buff += (char)payload[i];
  }
  Serial.print(Mqtt_Buff);
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if (Mqtt_Buff == "ledon") {//如果接收字符ledon，亮灯
    turnOnLed();//关灯函数
  } else if (Mqtt_Buff == "ledoff") {//如果接收字符ledoff，灭灯
    turnOffLed();//关灯函数
  } else if (Mqtt_Buff == "fanon") {//如果接收字符fanon，开风扇
    Mode = Maul;
    turnOnFan();//开风扇函数
  } else if (Mqtt_Buff == "fanoff") {//如果接收字符fanoff，关风扇
    Mode = Auto;
    turnOffFan();//关风扇函数
  }
  Mqtt_Buff = "";
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(ID_MQTT)) {//连接mqtt
      Serial.println("connected");
      client.subscribe(ledtopic);//修改，修改为你的主题
      client.subscribe(fantopic);//修改，修改为你的主题
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(B_led, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(Sensor_DO, INPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(callback);
  digitalWrite(B_led, LOW);//开机先关灯
  digitalWrite(relay, LOW);//开机继电器断开
}
void loop() {
  if (!client.connected()) {//判断mqtt是否连接
    reconnect();
  }
  client.loop();//mqtt客户端


  long now = millis();//获取当前时间戳
  if (now - lastMsg > timeval) {//如果达到3s，进行数据上传
    lastMsg = now;

    DHT.read(DHT11_PIN);//DHT11读取环境的温湿度数据
    /*串口打印数据*/
    Serial.print("DHT.temperature=");
    Serial.println(DHT.temperature);
    Serial.print("DHT.humidity=");
    Serial.println(DHT.humidity);

    sensorValue = analogRead(Sensor_AO);
    Serial.print("Sensor AD Value = ");
    Serial.println(sensorValue);

    String  msg = "#" + (String)DHT.temperature + "#" + (String)DHT.humidity + "#" + (String)sensorValue + "#" + ledstatus + "#" + fanstatus + "#" + Alarmstatus; //数据封装#温度#湿度#烟感#灯、风扇开关状态#报警
    client.publish(dhttopic, msg.c_str());//数据上传

  }

  if (Mode == Auto) {
    if (digitalRead(Sensor_DO) == LOW)
    {
      Alarmstatus = "Alarm!";
      Serial.println("Alarm!");
      delay(3000);
      digitalWrite(relay, HIGH);
      digitalWrite(B_led, HIGH);
      delay(100);
      digitalWrite(B_led, LOW);
      delay(100);
    } else {
      Alarmstatus = "no Alarm!";
      digitalWrite(relay, LOW);
    }
  }
}

//打开灯泡
void turnOnLed() {
  ledstatus = "ledon";
  Serial.println("turn on light");
  digitalWrite(B_led, HIGH);
}
//关闭灯泡
void turnOffLed() {
  ledstatus = "ledoff";
  Serial.println("turn off light");
  digitalWrite(B_led, LOW);
}
//打开风扇
void turnOnFan() {
  fanstatus = "fanon";
  Serial.println("turn on fan");
  delay(100);
  digitalWrite(relay, HIGH);
}
//关闭风扇
void turnOffFan() {
  fanstatus = "fanoff";
  Serial.println("turn off fan");
  delay(100);
  digitalWrite(relay, LOW);
}
