#include <SPI.h>
#include <Ethernet2.h>
#include "utility/w5500.h"
#include <PubSubClient.h>
#include <avr/wdt.h>
#include <TextFinder.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>

byte mac[]    = {  0xC4, 0xE7, 0xC4, 0x3E, 0x8E, 0x19 }; //MAC адрес контроллера
byte mqtt_serv[] = {192, 168, 1, 190}; //IP MQTT брокера

const char htmlx0[] PROGMEM = "<html><title>Controller IO setup Page</title><body marginwidth=\"0\" marginheight=\"0\" ";
const char htmlx1[] PROGMEM = "leftmargin=\"0\" \"><table bgcolor=\"#999999\" border";
const char htmlx2[] PROGMEM = "=\"0\" width=\"100%\" cellpadding=\"1\" ";
const char htmlx3[] PROGMEM = "\"><tr><td>&nbsp Controller IO setup Page</td></tr></table><br>";
const char* const string_table0[] PROGMEM = {htmlx0, htmlx1, htmlx2, htmlx3};

const char htmla0[] PROGMEM = "<script>function hex2num (s_hex) {eval(\"var n_num=0X\" + s_hex);return n_num;}";
const char htmla1[] PROGMEM = "</script><table><form><input type=\"hidden\" name=\"SBM\" value=\"1\"><tr><td>MAC:&nbsp&nbsp&nbsp";
const char htmla2[] PROGMEM = "<input id=\"T1\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT1\" value=\"";
const char htmla3[] PROGMEM = "\">.<input id=\"T3\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT2\" value=\"";
const char htmla4[] PROGMEM = "\">.<input id=\"T5\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT3\" value=\"";
const char htmla5[] PROGMEM = "\">.<input id=\"T7\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT4\" value=\"";
const char htmla6[] PROGMEM = "\">.<input id=\"T9\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT5\" value=\"";
const char htmla7[] PROGMEM = "\">.<input id=\"T11\" type=\"text\" size=\"1\" maxlength=\"2\" name=\"DT6\" value=\"";
const char* const string_table1[] PROGMEM = {htmla0, htmla1, htmla2, htmla3, htmla4, htmla5, htmla6, htmla7};

const char htmlb0[] PROGMEM = "\"><input id=\"T2\" type=\"hidden\" name=\"DT1\"><input id=\"T4\" type=\"hidden\" name=\"DT2";
const char htmlb1[] PROGMEM = "\"><input id=\"T6\" type=\"hidden\" name=\"DT3\"><input id=\"T8\" type=\"hidden\" name=\"DT4";
const char htmlb2[] PROGMEM = "\"><input id=\"T10\" type=\"hidden\" name=\"DT5\"><input id=\"T12\" type=\"hidden\" name=\"D";
const char htmlb3[] PROGMEM = "T6\"></td></tr><tr><td>MQTT: <input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT7\" value=\"";
const char htmlb4[] PROGMEM = "\">.<input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT8\" value=\"";
const char htmlb5[] PROGMEM = "\">.<input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT9\" value=\"";
const char htmlb6[] PROGMEM = "\">.<input type=\"text\" size=\"1\" maxlength=\"3\" name=\"DT10\" value=\"";
const char* const string_table2[] PROGMEM = {htmlb0, htmlb1, htmlb2, htmlb3, htmlb4, htmlb5, htmlb6};

const char htmlc0[] PROGMEM = "\"></td></tr><tr><td><br></td></tr><tr><td><input id=\"button1\"type=\"submit\" value=\"SAVE\" ";
const char htmlc1[] PROGMEM = "></td></tr></form></table></body></html>";
const char* const string_table3[] PROGMEM = {htmlc0, htmlc1};

const char htmld0[] PROGMEM = "Onclick=\"document.getElementById('T2').value ";
const char htmld1[] PROGMEM = "= hex2num(document.getElementById('T1').value);";
const char htmld2[] PROGMEM = "document.getElementById('T4').value = hex2num(document.getElementById('T3').value);";
const char htmld3[] PROGMEM = "document.getElementById('T6').value = hex2num(document.getElementById('T5').value);";
const char htmld4[] PROGMEM = "document.getElementById('T8').value = hex2num(document.getElementById('T7').value);";
const char htmld5[] PROGMEM = "document.getElementById('T10').value = hex2num(document.getElementById('T9').value);";
const char htmld6[] PROGMEM = "document.getElementById('T12').value = hex2num(document.getElementById('T11').value);\"";
const char* const string_table4[] PROGMEM = {htmld0, htmld1, htmld2, htmld3, htmld4, htmld5, htmld6};

#define ID_CONNECT "counter"

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
#define CNT1_ADR        110
#define CNT2_ADR        120
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


unsigned long cnt_1 = 0;
unsigned long cnt_2 = 0;
unsigned long prev_cnt_1 = 2;
unsigned long prev_cnt_2 = 2;
unsigned long prev_c1 = 2;
unsigned long prev_c2 = 2;
unsigned long chk = 0;
unsigned long chk_S = 0;
unsigned long prevMillis = 0;
unsigned long prevMillis_cnt1 = 0;
unsigned long prevMillis_cnt2 = 0;
unsigned long lastReconnect = 0;
bool namur = false;
unsigned long poll = 5000;
int ratio = 1; //Множитель от 1 до 32767
int interrupt_1 = RISING;
int interrupt_2 = RISING;
int nmr_lvl_1 = 512;
int nmr_brk_1 = 200;
int nmr_lvl_2 = 512;
int nmr_brk_2 = 200;
int Ain_1 = 0;
int Ain_2 = 0;
int prevAin_1;
int prevAin_2;
int bounce = 500;
int error = 0;
char b[50];
char s[16];
char buff[50];
int a = 0;
const byte ID = 0x91;
char buffer[100];

////////////////////////////////////////////////////////////////////////////
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  String strTopic = String(topic);
  String strPayload = String((char*)payload);
  callback_iobroker(strTopic, strPayload);
}

EthernetClient ethClient;
EthernetServer http_server(80);
PubSubClient mqtt(ethClient);

boolean reconnect() {
  digitalWrite(LED, !digitalRead(LED));
  digitalWrite(LED, !digitalRead(LED));
  if (mqtt.connect(ID_CONNECT)) {
    Public();
    digitalWrite(LED, LOW);
  }
  return mqtt.connected();
}

void setup() {
  MCUSR = 0;
  wdt_disable();
  pinMode(LED, OUTPUT);
  pinMode(IN_1, INPUT);
  pinMode(IN_2, INPUT);
  pinMode(CTRL_D1, OUTPUT);
  pinMode(CTRL_D2, OUTPUT);
  pinMode(CTRL_A1, OUTPUT);
  pinMode(CTRL_A2, OUTPUT);
  for (int i = 0 ; i < 100; i++) {
    digitalWrite(LED, !digitalRead(LED));
    delay(200 - i);
  }
  digitalWrite(LED, LOW);
  //Serial.begin(9600);
  if (EEPROM.read(200) != 88) { //Если первый запуск
    EEPROM.write(200, 88);
    reboot();
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
  }

  prev_cnt_1 = cnt_1;
  prev_cnt_2 = cnt_2;

  if (!namur) {
    namurOFF();
  } else {
    namurON();
  }

  httpSetup();
  mqttSetup();
  delay(1000);
  w5500.setRetransmissionTime(0x01F4);
  w5500.setRetransmissionCount(3);
  wdt_enable(WDTO_8S);

  if (mqtt.connect(ID_CONNECT)) {
    Public();
  }
}

void loop() {
  wdt_reset();
  checkHttp();
  if (!mqtt.connected()) {
    if (millis() - lastReconnect > 10000) {
      lastReconnect = millis();
      if (Ethernet.begin(mac) == 0) {
        Reset();
      } else {
        if (reconnect()) {
          lastReconnect = 0;
        }
      }
    }
  } else {
    mqtt.loop();
  }

  if (analogRead(PWR_CTRL) < 1000) {
    digitalWrite(LED, LOW);
    save();
  }

  if (namur) {
    ReadNamur();
  } else {
    ReadCount();
  }

  if (millis() - prevMillis >= poll) {
    wdt_reset();
    prevMillis = millis();
    if (namur) {
      mqtt.publish("myhome/counter/A_1", IntToChar(analogRead(AIN_1))); /**/
      mqtt.publish("myhome/counter/A_2", IntToChar(analogRead(AIN_2))); /**/
    }
    if (cnt_1 != prev_cnt_1) {
      prev_cnt_1 = cnt_1;
      mqtt.publish("myhome/counter/count_1", LongToChar(cnt_1));
    }
    if (cnt_2 != prev_cnt_2) {
      prev_cnt_2 = cnt_2;
      mqtt.publish("myhome/counter/count_2", LongToChar(cnt_2));
    }
    if (error > 0) {
      const char* e = "err";
      switch (error) {
        case 1:
          e = "CHK SUMM";
          break;
        case 2:
          e = "Break input 1";
          break;
        case 3:
          e = "Break input 2";
          break;
      }
      mqtt.publish("myhome/counter/error", e);
    } else {
      mqtt.publish("myhome/counter/error", " ");
    }
  }
}

void ReadCount() {
  int c1 = digitalRead(IN_1);
  int c2 = digitalRead(IN_2);
  if (millis() - prevMillis_cnt1 >= bounce) {
    prevMillis_cnt1 = millis();
    if (c1 != prev_c1) {
      digitalWrite(LED, !digitalRead(LED));
      prev_c1 = c1;
      switch (interrupt_1) {
        case 1:
          cnt_1++;
          break;
        case 2:
          if (c1 == HIGH) {
            cnt_1++;
          }
          break;
        case 3:
          if (c1 == LOW) {
            cnt_1++;
          }
          break;
      }
    }
  }
  if (millis() - prevMillis_cnt2 >= bounce) {
    prevMillis_cnt2 = millis();
    if (c2 != prev_c2) {
      digitalWrite(LED, !digitalRead(LED));
      prev_c2 = c2;
      switch (interrupt_2) {
        case 1:
          cnt_2++;
          break;
        case 2:
          if (c2 == HIGH) {
            cnt_2++;
          }
          break;
        case 3:
          if (c2 == LOW) {
            cnt_2++;
          }
          break;
      }
    }
  }
}

char* LongToChar (unsigned long a) {
  int n = sprintf (buff, "%lu", a);
  return buff;
}
const char* IntToChar (unsigned int v) {
  sprintf(buff, "%d", v);
  return buff;
}

void Public() {
  char s[16];
  sprintf(s, "%d.%d.%d.%d", Ethernet.localIP()[0], Ethernet.localIP()[1], Ethernet.localIP()[2], Ethernet.localIP()[3]);
  mqtt.publish("myhome/counter/count_1", LongToChar(cnt_1 * (long)ratio));
  mqtt.publish("myhome/counter/count_2", LongToChar(cnt_2 * (long)ratio));
  mqtt.publish("myhome/counter/config/ip", s);
  mqtt.publish("myhome/counter/error", "");
  mqtt.publish("myhome/counter/config/bounce", IntToChar(bounce));
  mqtt.publish("myhome/counter/config/namur_lvl_1", IntToChar(nmr_lvl_1));
  mqtt.publish("myhome/counter/config/namur_brk_1", IntToChar(nmr_brk_1));
  mqtt.publish("myhome/counter/config/namur_lvl_2", IntToChar(nmr_lvl_2));
  mqtt.publish("myhome/counter/config/namur_brk_2", IntToChar(nmr_brk_2));
  mqtt.publish("myhome/counter/config/interrupt_1", IntToChar(interrupt_1));
  mqtt.publish("myhome/counter/config/interrupt_2", IntToChar(interrupt_2));
  mqtt.publish("myhome/counter/config/ratio", IntToChar(ratio));
  mqtt.publish("myhome/counter/config/namur", BoolToChar(namur));
  mqtt.publish("myhome/counter/config/polling", LongToChar(poll));
  mqtt.publish("myhome/counter/save", "false");
  mqtt.publish("myhome/counter/config/reset", "false");
  mqtt.publish("myhome/counter/correction", "0;0");
  mqtt.publish("myhome/counter/connection", "true");
  mqtt.subscribe("myhome/counter/#");
}

void mqttSetup() {
  int idcheck = EEPROM.read(0);
  if (idcheck == ID) {
    for (int i = 0; i < 4; i++) {
      mqtt_serv[i] = EEPROM.read(i + 7);
    }
  }
  mqtt.setServer(mqtt_serv, 1883);
  mqtt.setCallback(callback);
}

void httpSetup() {
  int idcheck = EEPROM.read(0);
  if (idcheck == ID) {
    for (int i = 0; i < 6; i++) {
      mac[i] = EEPROM.read(i + 1);
    }
  }
  Ethernet.begin(mac);
}

void checkHttp() {
  EthernetClient http = http_server.available();
  if (http) {
    TextFinder  finder(http );
    while (http.connected()) {
      if (http.available()) {
        if ( finder.find("GET /") ) {
          if (finder.findUntil("setup", "\n\r")) {
            if (finder.findUntil("SBM", "\n\r")) {
              byte SET = finder.getValue();
              while (finder.findUntil("DT", "\n\r")) {
                int val = finder.getValue();
                if (val >= 1 && val <= 6) {
                  mac[val - 1] = finder.getValue();
                }
                if (val >= 7 && val <= 10) {
                  mqtt_serv[val - 7] = finder.getValue();
                }
              }
              for (int i = 0 ; i < 6; i++) {
                EEPROM.write(i + 1, mac[i]);
              }
              for (int i = 0 ; i < 4; i++) {
                EEPROM.write(i + 7, mqtt_serv[i]);
              }
              EEPROM.write(0, ID);
              http.println("HTTP/1.1 200 OK");
              http.println("Content-Type: text/html");
              http.println();
              for (int i = 0; i < 4; i++) {
                strcpy_P(buffer, (char*)pgm_read_word(&(string_table0[i])));
                http.print( buffer );
              }
              http.println();
              http.print("Saved!");
              http.println();
              http.print("Restart");
              for (int i = 1; i < 10; i++) {
                http.print(".");
                delay(500);
              }
              http.println("OK");
              Reset(); // ребутим с новыми параметрами
            }
            http.println("HTTP/1.1 200 OK");
            http.println("Content-Type: text/html");
            http.println();
            for (int i = 0; i < 4; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table0[i])));
              http.print( buffer );
            }
            for (int i = 0; i < 3; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[i])));
              http.print( buffer );
            }
            http.print(mac[0], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[3])));
            http.print( buffer );
            http.print(mac[1], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[4])));
            http.print( buffer );
            http.print(mac[2], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[5])));
            http.print( buffer );
            http.print(mac[3], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[6])));
            http.print( buffer );
            http.print(mac[4], HEX);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table1[7])));
            http.print( buffer );
            http.print(mac[5], HEX);
            for (int i = 0; i < 4; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[i])));
              http.print( buffer );
            }
            http.print(mqtt_serv[0], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[4])));
            http.print( buffer );
            http.print(mqtt_serv[1], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[5])));
            http.print( buffer );
            http.print(mqtt_serv[2], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table2[6])));
            http.print( buffer );
            http.print(mqtt_serv[3], DEC);
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[0])));
            http.print( buffer );
            for (int i = 0; i < 7; i++) {
              strcpy_P(buffer, (char*)pgm_read_word(&(string_table4[i])));
              http.print( buffer );
            }
            strcpy_P(buffer, (char*)pgm_read_word(&(string_table3[1])));
            http.print( buffer );
            break;
          }
        }
        http.println("HTTP/1.1 200 OK");
        http.println("Content-Type: text/html");
        http.println();
        http.print("IOT controller [");
        http.print(ID_CONNECT);
        http.print("]: go to <a href=\"/setup\"> setup</a>");
        break;
      }
    }
    delay(1);
    http.stop();
  } else {
    return;
  }
}

void Reset() {
  for (;;) {}
}
