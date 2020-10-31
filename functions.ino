void shwDebug(String text) {
  if (DEBUG) Serial.println (text);
}
boolean lowBattery (float v_bat){
  //if (v_bat > 5000)    { //3492
  if (v_bat < 3492)    { //
    lcd.clearDisplay();
    lcd.setTextColor(BLACK);
    lcd.setCursor(0,12);        
    lcd.setTextSize(2);
    lcd.print("LOW BAT");
    if (backlight) analogWrite(BACKLIGTH, 0);
    if (screen==1||screen==2) digitalWrite(sensVCC, HIGH);
    lcd.display();
    return true;
  }
  return false;
}
void powerSave (){
  if (millis() - timer >= PERIOD) {   
      lcd.clearDisplay();
      lcd.setTextColor(BLACK);
      lcd.setCursor(12,12);        
      lcd.setTextSize(1);
      lcd.print("POWER SAVE");
      if (backlight) analogWrite(BACKLIGTH, 0);
      if (screen==1||screen==2) digitalWrite(sensVCC, LOW);
      lcd.display();
      screen=0;
    }
}
void drawBatteryState(float v_bat) {
  lcd.fillRect(68, 0, 16, 7, WHITE);
  lcd.drawRect(83, 2, 1, 3, BLACK);
  lcd.drawRoundRect(68, 0, 15, 7, 2, BLACK);
  // 3, 44  4, 2 0, 76  0, 152
  //4,200 full  4
  //4,048       3
  //3,896       2
  //3,744       1
  //3,592       0
  //3,440 zero  -

  if (v_bat > 4500)   {
    lcd.fillRect(70, 2, 10, 3, BLACK);
  }
  if (v_bat > 4048)   {
    lcd.drawRect(79, 2, 2, 3, BLACK);
  }
  if (v_bat > 3896)   {
    lcd.drawRect(76, 2, 2, 3, BLACK);
  }
  if (v_bat > 3744)    {
    lcd.drawRect(73, 2, 2, 3, BLACK);
  }
  if (v_bat > 3592)    {
    lcd.drawRect(70, 2, 2, 3, BLACK);
  }
  
}
long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(75); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  //scale_constant = internal1.1Ref * 1023 * 1000
  //где
  //internal1.1Ref = 1.1 * Vcc1 (показания_вольтметра) / Vcc2 (показания_функции_readVcc())
  //4.967/4600-------1215079.369565217
  // result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*100000
  result = 1132060 / result;

  return result; // Vcc in millivolts
  //http://blog.unlimite.net/?p=25
}


// медианный фильтр из 3ёх значений
/* https://alexgyver.ru/lessons/filters/#%D0%BC%D0%B5%D0%B4%D0%B8%D0%B0%D0%BD%D0%BD%D1%8B%D0%B9-%D1%84%D0%B8%D0%BB%D1%8C%D1%82%D1%80 */
float middle_of_3(float a, float b, float c) {
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  }
  else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    }
    else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}

/* Recode russian fonts from UTF-8 to Windows-1251 */
String utf8rus(String source){
  int i,k;
  String target;
  unsigned char n;
  char m[2] = { '0', '\0' };

  k = source.length(); i = 0;

  while (i < k) {
    n = source[i]; i++;

    if (n >= 0xC0) {
      switch (n) {
        case 0xD0: {
          n = source[i]; i++;
          if (n == 0x81) { n = 0xA8; break; }
          if (n >= 0x90 && n <= 0xBF) n = n + 0x30;
          break;
        }
        case 0xD1: {
          n = source[i]; i++;
          if (n == 0x91) { n = 0xB8; break; }
          if (n >= 0x80 && n <= 0x8F) n = n + 0x70;
          break;
        }
      }
    }
    m[0] = n; target = target + String(m);
  }
return target;
}
void beep(unsigned char delayms){       
  if (buzzer) {
    tone(BuzzerPIN, 450);
    delay(delayms);
    noTone(BuzzerPIN);
    delay(delayms);
  }
  else delay(50);
}
