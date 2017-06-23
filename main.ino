#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <avr/wdt.h>

// Update these with values suitable for your network.
byte mac[]    = {  0xC4, 0xE7, 0xC4, 0x3E, 0x3E, 0x12 };

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

#define CHK_ADR         100
#define CNT1_ADR        10
#define CNT2_ADR        20
#define POLL_ADR        30
#define NAMUR_ADR       40
#define RATIO_ADR       50
#define INTERRUPT_1_ADR 60
#define INTERRUPT_2_ADR 62
#define NMR_LVL1_ADR    70
#define NMR_BRK1_ADR    72
#define NMR_LVL2_ADR    74
#define NMR_BRK2_ADR    76
#define BOUNCE_ADR      78


volatile unsigned long cnt_1 = 0;
volatile unsigned long cnt_2 = 0;
unsigned long prev_cnt_1 = 2;
unsigned long prev_cnt_2 = 2;
unsigned long chk = 0;
unsigned long chk_S = 0;
long prevMillis = 0;
long prevMillis_cnt = 0;
bool namur = false;
unsigned long poll = 5000;
int ratio = 1; //Множитель от 1 до 32767
int interrupt_1 = FALLING;
int interrupt_2 = FALLING;
int nmr_lvl_1 = 512;
int nmr_brk_1 = 200;
int nmr_lvl_2 = 512;
int nmr_brk_2 = 200;
int Ain_1 = 0;
int Ain_2 = 0;
int prevAin_1;
int prevAin_2;
int bounce = 0;
bool error = 0;


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
  wdt_reset();
  while (!client.connected()) {
    if (client.connect(id_connect)) {
      Public();
      digitalWrite(LED, LOW);
    } else {
      delay(5000);
    }
  }
}

byte ip[4] = {192, 168, 1, 171};
byte server[4] = {192, 168, 1, 190};

void setup() {
  //IPAddress ip(192, 168, 1, 171);
  //IPAddress server(192, 168, 1, 190);
  
  MCUSR = 0;
  wdt_disable();
  //Serial.begin(9600);
  if (EEPROM.read(1) != 88) { //Если первый запуск
    EEPROM.write(1, 88);
    for (int i = 110 ; i < 114; i++){
       EEPROM.write(i, ip[i]);
    }
    for (int i = 120 ; i < 124; i++){
       EEPROM.write(i, ip[i]);
    }
  } else {
    chk = EEPROMReadLong(CHK_ADR);
    cnt_1 = EEPROMReadLong(CNT1_ADR);
    cnt_2 = EEPROMReadLong(CNT2_ADR);
    poll = EEPROMReadLong(POLL_ADR);
    namur = IntToBool(EEPROM.read(NAMUR_ADR));
    ratio = EEPROM.read(RATIO_ADR);
    interrupt_1 = EEPROM.read(INTERRUPT_1_ADR);
    interrupt_2 = EEPROM.read(INTERRUPT_2_ADR);
    nmr_lvl_1 = EEPROMReadInt(NMR_LVL1_ADR);
    nmr_brk_1 = EEPROMReadInt(NMR_BRK1_ADR);
    nmr_lvl_2 = EEPROMReadInt(NMR_LVL2_ADR);
    nmr_brk_2 = EEPROMReadInt(NMR_BRK2_ADR);
    bounce = EEPROMReadInt(BOUNCE_ADR);
    chk_S = namur + ratio + interrupt_1 + interrupt_2 + nmr_brk_2 + nmr_brk_1;
    if (chk != chk_S) {
      error = 1;
    }
    for (int i = 110; i < 114; i++){
      ip[i] = EEPROM.read(i);
    }
    for (int i = 120; i < 124; i++){
      server[i] = EEPROM.read(i);
    }
  }

  prev_cnt_1 = cnt_1;
  prev_cnt_2 = cnt_2;

  pinMode(IN_1, INPUT);
  pinMode(IN_2, INPUT);
  pinMode(CTRL_D1, OUTPUT);
  pinMode(CTRL_D2, OUTPUT);
  pinMode(CTRL_A1, OUTPUT);
  pinMode(CTRL_A2, OUTPUT);
  pinMode(LED, OUTPUT);

  if (!namur) {
    namurOFF();
    attachInterrupt(0, Count_1, interrupt_1);
    attachInterrupt(1, Count_2, interrupt_2);
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

void loop() {
  wdt_reset();
  client.loop();
  if (!client.connected()) {
    reconnect();
  }

  if (analogRead(PWR_CTRL) < 1000) {
    digitalWrite(LED, LOW);
    save();
  }

  if (namur) {
    int in_1 = analogRead(AIN_1);
    int in_2 = analogRead(AIN_2);
    if (in_1 > nmr_brk_1 && in_1 > nmr_lvl_1) {
      Ain_1 = 1;
    } else if (in_1 > nmr_brk_1 && in_1 < nmr_lvl_1) {
      Ain_1 = 0;
    } else if (in_1 < nmr_brk_1) {
      error = 2;
    }
    if (in_2 > nmr_brk_2 && in_2 > nmr_lvl_2) {
      Ain_2 = 1;
    } else if (in_2 > nmr_brk_2 && in_2 < nmr_lvl_2) {
      Ain_2 = 0;
    } else if (in_2 < nmr_brk_2) {
      error = 3;
    }
    if (Ain_1 != prevAin_1) {
      prevAin_1 = Ain_1;
      switch (interrupt_1) {
        case 1:
          cnt_1++;
          break;
        case 2:
          if (Ain_1 == 1) {
            cnt_1++;
          }
          break;
        case 3:
          if (Ain_1 == 0) {
            cnt_1++;
          }
          break;
      }
      digitalWrite(LED, !digitalRead(LED));
    }
    if (Ain_2 != prevAin_2) {
      prevAin_2 = Ain_2;
      switch (interrupt_2) {
        case 1:
          cnt_2++;
          break;
        case 2:
          if (Ain_2 == 1) {
            cnt_2++;
          }
          break;
        case 3:
          if (Ain_2 == 0) {
            cnt_2++;
          }
          break;
      }
      digitalWrite(LED, !digitalRead(LED));
    }
  }

  if (millis() - prevMillis >= poll) {
    wdt_reset();
    prevMillis = millis();
    if (namur) {
      client.publish("myhome/counter/A_1", IntToChar(analogRead(AIN_1))); /**/
      client.publish("myhome/counter/A_2", IntToChar(analogRead(AIN_2))); /**/
    }
    if (cnt_1 != prev_cnt_1 || cnt_2 != prev_cnt_2) {
      prev_cnt_1 = cnt_1;
      prev_cnt_2 = cnt_2;
      client.publish("myhome/counter/count_1", IntToChar(cnt_1 * ratio));
      client.publish("myhome/counter/count_2", IntToChar(cnt_2 * ratio));
    }
    if (error > 0) {
      char* e = "err";
      switch (error) {
        case 1:
          e = "CHK SUMM";
          break;
        case 2:
          e = "Break Analog input 1";
          break;
        case 3:
          e = "Break Analog input 2";
          break;
      }
      client.publish("myhome/counter/error", e);
    } else {
      client.publish("myhome/counter/error", " ");
    }
  }

}

void Public() {
  char s[16];  
  sprintf(s, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  client.publish("myhome/counter/config/ip", s);
  client.publish("myhome/counter/error", " ");
  client.publish("myhome/counter/count_1", IntToChar(cnt_1 * ratio));
  client.publish("myhome/counter/count_2", IntToChar(cnt_2 * ratio));
  client.publish("myhome/counter/config/bounce", IntToChar(bounce));
  client.publish("myhome/counter/config/namur_lvl_1", IntToChar(nmr_lvl_1));
  client.publish("myhome/counter/config/namur_brk_1", IntToChar(nmr_brk_1));
  client.publish("myhome/counter/config/namur_lvl_2", IntToChar(nmr_lvl_2));
  client.publish("myhome/counter/config/namur_brk_2", IntToChar(nmr_brk_2));
  client.publish("myhome/counter/config/interrupt_1", IntToChar(interrupt_1));
  client.publish("myhome/counter/config/interrupt_2", IntToChar(interrupt_2));
  client.publish("myhome/counter/config/ratio", IntToChar(ratio));
  client.publish("myhome/counter/config/namur", BoolToChar(namur));
  client.publish("myhome/counter/config/polling", IntToChar(poll));
  client.publish("myhome/counter/save", "false");
  client.publish("myhome/counter/correction", "0;0");
  client.publish("myhome/counter/connection", "true");
  client.subscribe("myhome/counter/#");
}

void Count_1() {
  detachInterrupt (0);
  if (millis() - prevMillis_cnt >= bounce){
    prevMillis_cnt = millis();
    cnt_1++;
    digitalWrite(LED, !digitalRead(LED));
  }
  attachInterrupt(0, Count_1, interrupt_1);
}
void Count_2() {
  detachInterrupt (1);
  if (millis() - prevMillis_cnt >= bounce){
    prevMillis_cnt = millis();
    cnt_2++;
    digitalWrite(LED, !digitalRead(LED));
  }
  attachInterrupt(1, Count_2, interrupt_2);
}

