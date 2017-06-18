#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// Update these with values suitable for your network.
byte mac[]    = {  0xC4, 0xE7, 0xC4, 0x3E, 0x3E, 0x12 };
IPAddress ip(192, 168, 1, 171);
IPAddress server(192, 168, 1, 190);

#define id_connect "counter"
#define Prefix_subscribe "myhome/counter/"

#define PWR_CTRL   A2 //Контроль напряжения
#define IN_1    2  //Первый вход
#define IN_2    3  //Второй вход
#define AIN_1   A0 //Первый аналоговый вход
#define AIN_2   A1 //Второй аналоговый вход
#define CTRL_D1 4  //Выход для управления коммутатором
#define CTRL_A1 5  //Выход для управления коммутатором
#define CTRL_D2 6  //Выход для управления коммутатором
#define CTRL_A2 7  //Выход для управления коммутатором
#define LED     9  //Светодиод

#define CHK_ADR   99
#define CNT1_ADR  10
#define CNT2_ADR  20
#define POLL_ADR  30
#define NAMUR_ADR 40

#define ratio 1 //Литры за один импульс *10
volatile unsigned long cnt_1 = 0;  // Начальные показания Холодной воды
volatile unsigned long cnt_2 = 0;  // Начальнаые показания Горячей воды
unsigned long chk = 0;
long prevMillis = 0;
bool namur = false;
long poll = 5000;

////////////////////////////////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {
    payload[length] = '\0';
    String strTopic = String(topic);
    String strPayload = String((char*)payload);
    callback_iobroker(strTopic, strPayload);
}

EthernetClient ethClient;
PubSubClient client(ethClient);

void reconnect() {
  digitalWrite(LED, !digitalRead(LED));
  while (!client.connected()) {
    wdt_reset();
    if (client.connect(id_connect)) {
      Public();
      digitalWrite(LED, LOW);
    } else {
      delay(5000);
    }
  }
}

void setup(){
  //Serial.begin(9600);
  if (EEPROM.read(1)!= 88 && EEPROM.read(90)!= 99){ //Если первый запуск
    EEPROM.write(1, 88);
    EEPROM.write(90, 99);
  } else {
    chk = EEPROMReadInt(CHK_ADR);
    cnt_1 = EEPROMReadInt(CNT1_ADR);
    cnt_2 = EEPROMReadInt(CNT2_ADR);
    poll = EEPROMReadInt(POLL_ADR);
    namur = IntToBool(EEPROM.read(NAMUR_ADR));
    if (chk != (cnt_1 - cnt_2)){
       client.publish("myhome/counter/error", "CHK");
    }
  }

  pinMode(IN_1, INPUT);
  pinMode(IN_2, INPUT);
  pinMode(CTRL_D1, OUTPUT);
  pinMode(CTRL_D2, OUTPUT);
  pinMode(CTRL_A1, OUTPUT);
  pinMode(CTRL_A2, OUTPUT);
  pinMode(LED, OUTPUT);
  
  if (!namur){
      namurOFF();
      attachInterrupt(0, Count_1, FALLING);
      attachInterrupt(1, Count_2, FALLING);
  } else {
      namurON();
  }
  
  client.setServer(server, 1883);
  client.setCallback(callback);
  Ethernet.begin(mac, ip);
  delay(100);
  wdt_enable(WDTO_8S);
  
  if (client.connect(id_connect)) {
    Public();
  }
}

void loop(){
  wdt_reset();
  client.loop();

  if (analogRead(PWR_CTRL) < 1000){
    digitalWrite(LED, LOW);
    save();
  }
  if (namur){
    //считаем по аналоговым
  }

  if (millis() - prevMillis > poll){
    prevMillis = millis();
    client.publish("myhome/counter/count_1", IntToChar(cnt_1));
    client.publish("myhome/counter/count_2", IntToChar(cnt_2));
    client.publish("myhome/counter/PWR_CTRL", IntToChar(analogRead(PWR_CTRL))); 
  }
  
  
  if (!client.connected()) {
    reconnect();
  }
}

void Public(){
  client.publish("myhome/counter/error", "false");
  client.publish("myhome/counter/config/namur", BoolToChar(namur));
  client.publish("myhome/counter/config/polling", IntToChar(poll));
  client.publish("myhome/counter/save", "false");
  client.publish("myhome/counter/correction", "");
  client.publish("myhome/counter/connection", "true");
  client.subscribe("myhome/counter/#");
}

void Count_1(){
  detachInterrupt (0);
  cnt_1++;
  digitalWrite(LED, !digitalRead(LED));
  attachInterrupt(0, Count_1, FALLING);
}
void Count_2(){
  detachInterrupt (1);
  cnt_2++;
  digitalWrite(LED, !digitalRead(LED));
  attachInterrupt(1, Count_2, FALLING);
}
