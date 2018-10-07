//--------------------------------- Functions ---------------------------------//
/////////////////////////////////От Брокера//////////////////////////////////////////////
void callback_iobroker(String strTopic, String strPayload){
  if (strTopic == "myhome/counter/correction") {
      cnt_1 = atol(strPayload.substring(0,strPayload.indexOf(';')).c_str()) / ratio;
      cnt_2 = atol(strPayload.substring(strPayload.lastIndexOf(';')+1).c_str()) / ratio;
      mqtt.publish("myhome/counter/correction", "0;0");
  }
  else if (strTopic == "myhome/counter/save") {
    if (strPayload == "true"){
      save();
      mqtt.publish("myhome/counter/save", "false");
    }
  }
  else if (strTopic == "myhome/counter/config/polling") {
      poll = strPayload.toInt();
      if (poll < 500){
        poll = 500;
      } else if (poll > 4294967295){
        poll = 4294967295;
      }
      EEPROMWriteLong(POLL_ADR, poll);  //Интервал публикации
      mqtt.publish("myhome/counter/config/polling", IntToChar(poll));
  }
  else if (strTopic == "myhome/counter/config/bounce") {
      bounce = strPayload.toInt();
      if (bounce < 0){
        bounce = 0;
      } else if (bounce > 5000){
        bounce = 5000;
      }
      EEPROMWriteInt(BOUNCE_ADR, bounce);  //Защита от дребезга
      mqtt.publish("myhome/counter/config/bounce", IntToChar(bounce));
  }
  else if (strTopic == "myhome/counter/config/namur") {
      namur = SrtToBool(strPayload);
      EEPROM.write(NAMUR_ADR, BoolToInt(namur));
      mqtt.publish("myhome/counter/config/namur", BoolToChar(namur));
      save();
      reboot();
  }
  else if (strTopic == "myhome/counter/config/reset") {
      if(strPayload == "true"){
        save();
        mqtt.publish("myhome/counter/config/reset", "false");
        reboot();
      }
  }
  else if (strTopic == "myhome/counter/config/ratio") {
      ratio = strPayload.toInt();
      if (ratio < 1){
        ratio = 1;
      } else if (ratio > 32767){
        ratio = 32767;
      }
      mqtt.publish("myhome/counter/config/ratio", IntToChar(ratio));
  }
  else if (strTopic == "myhome/counter/config/interrupt_1") {
      interrupt_1 = strPayload.toInt();
      if (interrupt_1 < 1){
        interrupt_1 = 1;
      } else if (ratio > 3){
        interrupt_1 = 3;
      }
      mqtt.publish("myhome/counter/config/interrupt_1", IntToChar(interrupt_1));
  }
  else if (strTopic == "myhome/counter/config/interrupt_2") {
      interrupt_2 = strPayload.toInt();
      if (interrupt_2 < 1){
        interrupt_2 = 1;
      } else if (ratio > 3){
        interrupt_2 = 3;
      }
      mqtt.publish("myhome/counter/config/interrupt_2", IntToChar(interrupt_2));
  }
  else if (strTopic == "myhome/counter/config/namur_lvl_1") {
      nmr_lvl_1 = strPayload.toInt();
      EEPROMWriteInt(NMR_LVL1_ADR, nmr_lvl_1);  //Namur высокий уровень
      mqtt.publish("myhome/counter/config/namur_lvl_1", IntToChar(nmr_lvl_1));
  }
  else if (strTopic == "myhome/counter/config/namur_brk_1") {
      nmr_brk_1 = strPayload.toInt();
      EEPROMWriteInt(NMR_BRK1_ADR, nmr_brk_1);  //Namur уровень обрыва
      mqtt.publish("myhome/counter/config/namur_brk_1", IntToChar(nmr_brk_1));
  }
  else if (strTopic == "myhome/counter/config/namur_lvl_2") {
      nmr_lvl_2 = strPayload.toInt();
      EEPROMWriteInt(NMR_LVL2_ADR, nmr_lvl_2);  //Namur высокий уровень
      mqtt.publish("myhome/counter/config/namur_lvl_2", IntToChar(nmr_lvl_2));
  }
  else if (strTopic == "myhome/counter/config/namur_brk_2") {
      nmr_brk_2 = strPayload.toInt();
      EEPROMWriteInt(NMR_BRK2_ADR, nmr_brk_2);  //Namur уровень обрыва
      mqtt.publish("myhome/counter/config/namur_brk_2", IntToChar(nmr_brk_2));
  }
}


void save(){
  chk = namur + ratio + interrupt_1 + interrupt_2 + nmr_brk_2 + nmr_brk_1;
  EEPROMWriteLong(CNT1_ADR, cnt_1);  //Пишем показания счетчика в eeprom из переменной
  EEPROMWriteLong(CNT2_ADR, cnt_2);  //Пишем показания счетчика в eeprom из переменной
  EEPROMWriteLong(POLL_ADR, poll);  //Интервал публикации
  EEPROM.write(NAMUR_ADR, BoolToInt(namur));  //тип счетчика
  EEPROM.write(RATIO_ADR, ratio);  //Множитель
  EEPROM.write(INTERRUPT_1_ADR, interrupt_1); //Вектор прерывания
  EEPROM.write(INTERRUPT_2_ADR, interrupt_2); //Вектор прерывания
  EEPROMWriteInt(BOUNCE_ADR, bounce);  //Защита от дребезга
  EEPROMWriteInt(NMR_LVL1_ADR, nmr_lvl_1);  //Namur высокий уровень
  EEPROMWriteInt(NMR_BRK1_ADR, nmr_brk_1);  //Namur уровень обрыва
  EEPROMWriteInt(NMR_LVL2_ADR, nmr_lvl_2);  //Namur высокий уровень
  EEPROMWriteInt(NMR_BRK2_ADR, nmr_brk_2);  //Namur уровень обрыва
  EEPROMWriteLong(CHK_ADR, chk); //чек
}

void reboot(){
  save();
  delay(500);
  wdt_enable(WDTO_1S);
  for(;;){}
}

int SrtToBool(String t){
    if (t == "false" || t == "0" || t == "off"){
      return false;
    } else {
      return true;
    }
}

const char* BoolToChar (bool r) {
    return r ? "true" : "false";
}

int BoolToInt(bool s){
    return s ? 1 : 0;
}
int IntToBool(int i){
    if (i == 1){
      return true;
    } else {
      return false;
    }
}

void namurON(){
  digitalWrite(CTRL_D1, LOW);
  digitalWrite(CTRL_D2, LOW);
  digitalWrite(CTRL_A1, HIGH);
  digitalWrite(CTRL_A2, HIGH);
}

void namurOFF(){
  digitalWrite(CTRL_D1, HIGH);
  digitalWrite(CTRL_D2, HIGH);
  digitalWrite(CTRL_A1, LOW);
  digitalWrite(CTRL_A2, LOW);
}

////////////////////////////////////ФУНКЦИИ ПАМЯТИ///////////////////////////////////////////
//кушаем аж 4 байта EEPROM
void EEPROMWriteLong(int p_address, unsigned long p_value){
    byte four = (p_value & 0xFF);
    byte three = ((p_value >> 8) & 0xFF);
    byte two = ((p_value >> 16) & 0xFF);
    byte one = ((p_value >> 24) & 0xFF);
    EEPROM.write(p_address, four);
    EEPROM.write(p_address + 1, three);
    EEPROM.write(p_address + 2, two);
    EEPROM.write(p_address + 3, one);
}
// считаем нашы 4 байта
unsigned long EEPROMReadLong(int p_address){
   long four = EEPROM.read(p_address);
   long three = EEPROM.read(p_address + 1);
   long two = EEPROM.read(p_address + 2);
   long one = EEPROM.read(p_address + 3);
   return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
 }

void EEPROMWriteInt(int p_address, int p_value){
     byte lowByte = ((p_value >> 0) & 0xFF);
     byte highByte = ((p_value >> 8) & 0xFF);
     EEPROM.write(p_address, lowByte);
     EEPROM.write(p_address + 1, highByte);
   }
unsigned int EEPROMReadInt(int p_address){
     byte lowByte = EEPROM.read(p_address);
     byte highByte = EEPROM.read(p_address + 1);
     return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
   }

void ReadNamur(){
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
      //mqtt.publish("myhome/counter/count_1", LongToChar(cnt_1));
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
      //mqtt.publish("myhome/counter/count_2", LongToChar(cnt_2));
      digitalWrite(LED, !digitalRead(LED));
    }
}
