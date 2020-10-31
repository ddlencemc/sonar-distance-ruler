//#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <NewPing.h>
#include "GyverButton.h"
#include <EEPROM.h>
#include <LowPower.h> /* https://github.com/rocketscream/Low-Power */
// -------НАСТРОЙКИ-------
// длина корпуса в сантиметрах, для переноса начала отсчёта в заднюю часть корпуса
float case_offset = 14;
// -------НАСТРОЙКИ-------

//Экран
Adafruit_PCD8544 lcd = Adafruit_PCD8544(13,11,12,10,9);
#define BACKLIGTH A5 //Подсветка

// сонар
#define ECHO 6
#define TRIG 7
#define sensVCC 8
NewPing sonar(TRIG, ECHO, 400);

// Кнопки
#define buttUpPIN 2
#define buttDownPIN 3
#define buttonCenterPIN 4 

GButton btnUp(buttUpPIN);
GButton btnDown(buttDownPIN);
GButton btnCenter(buttonCenterPIN);

//GButton btnDown(buttDownPIN, LOW_PULL, NORM_OPEN);

//Пищалка
#define BuzzerPIN 5

#define DEBUG 1
#define PERIOD 300000

float dist_3[3] = {0.0, 0.0, 0.0};   // массив для хранения трёх последних измерений
float middle, dist, dist_filtered, lastDist, currentDist, partronikDist;
byte parktronikAction = 1;
float k;
byte i, delta, metr, cmetr;
unsigned long dispIsrTimer, sensTimer;

boolean backlight = true;
boolean buzzer = true;
boolean singleClick = true;
byte screen = 0;
byte item = 1;
byte enter= 0;
byte lastScreen = 0;
boolean roulette = true;
boolean partronik = false;
uint32_t timer = 0;


String Screens[5][4] = {
/*[0]*/{"Меню",
          "Рулетка",
          "Парктроник",
          "Настройки"
       },
/*[1]*/{"Рулетка",
          "--,--", 
          "см",
          "00.00, 00.00/Пл:"
       },
/*[2]*/{"Парктроник",
          "--,--", 
          "см",
          "м"
       },
/*[3]*/{"Настройки",
          "Подсветка:",
          "Звук:",
          "Назад"
       },
/*[4]*/{"Да","Нет","Пл:"," м2"},
};

byte Positions[][3] = {
  {30,0,1}, //Меню
  {0,9,1},  //Рулетка
  {0,18,1}, //Парктроник
  {0,27,1}, //Настройки
  
  {15,0,1}, //Рулетка
  {0,12,2}, //--,--
  {72,19,1},//см
  {0,28,1}, //00.00, 00.00/Пл:

  {10,0,1}, //Парктроник
  {0,12,2}, //++,++
  {72,19,1},//см
  {0,28,1}, //00.00, 00.00
  
  {10,0,1}, //Настройки
  {0,9,1},  //Подсветка
  {0,18,1}, //Звук
  {0,27,1}, //Назад
};

byte Routes [][4] = {
/* scr,itm,nxtscr,enter*/
  {0,1,1,0},
  {0,2,2,0},
  {0,3,3,0},

  {1,1,1,1},
  {2,1,2,1},
  
  {3,1,3,1},
  {3,2,3,1},
  {3,3,0,0},
} ;


void lcdShowLogo(){
  //Настройка экрана
  lcd.begin();
  lcd.cp437(true);
  lcd.setContrast(60);
  lcd.display();
  //if (buzzer) delay(1000);
}

void selectedItem (boolean select){
  lcd.setTextColor(BLACK);
  if (select) {lcd.setTextColor(WHITE, BLACK);lcd.print(">");}    
}

void lcdDrawScreen(){
  lcd.clearDisplay();
  drawBatteryState(readVcc());
  if (!partronik) digitalWrite(sensVCC, LOW);
  byte d = 0;
  for (int scr = 0; scr < 5; scr++) {
    for (int itm = 0; itm < 4; itm++) { 
      if (screen == scr) {
        lcd.setTextColor(BLACK);
        lcd.setCursor(Positions[d][0],Positions[d][1]);        
        lcd.setTextSize(Positions[d][2]);          
        selectedItem((screen == 0 || screen == 3)?(item==itm):0);
        /*Рулетка*/
        if (scr == 1) {  
          singleClick = false;
          roulette = true;
          digitalWrite(sensVCC, HIGH);
          if (itm==1 || itm==3) {
            if (itm==1) { 
              lastDist = currentDist;
              lcd.print((enter==1)?String(getDistance()):Screens[scr][itm]);
              //enter = 0;
            }
            if (itm==3){
              lcd.println(String(currentDist)+", "+ String(lastDist));
              lcd.print(Screens[4][2]+String((currentDist*lastDist)/10000) + Screens[4][3] );
            }              
          } else lcd.print(Screens[scr][itm]);             
        }
        else /*Парктроник*/
        if (scr == 2) {  
          singleClick = false;
          roulette = false;
          digitalWrite(sensVCC, HIGH);
          if (itm==1 || itm==3) {
            if (itm==1) { 
              if (enter==1){
                  parktronikAction=(parktronikAction==3)?parktronikAction:parktronikAction+1;
                  partronik = (parktronikAction==3) ;
                }
              if (partronik) {
                byte potDist = metr*100+cmetr;
                byte minDist =  potDist / 2;
                byte critDist = potDist / 3;
                if(partronikDist <= potDist && partronikDist > minDist && partronikDist > critDist){
                  beep(75); 
                  beep(75); 
                }else if (partronikDist < minDist && partronikDist > critDist){
                  beep(50); 
                  beep(50); 
                }else if (partronikDist <= critDist){
                  beep(1000); 
                }
              }
              lcd.print((partronik)?String(partronikDist):Screens[scr][itm]);              
              enter = 0;
            }
            if (itm==3){
              lcd.setTextColor(BLACK);
              if (parktronikAction==1)lcd.setTextColor(WHITE, BLACK); 
              lcd.print(String(metr)+Screens[2][3]+" ");
              lcd.setTextColor(BLACK);
              if (parktronikAction==2)lcd.setTextColor(WHITE, BLACK);
              lcd.print(String(cmetr)+Screens[2][2]);
            }              
          } else lcd.print(Screens[scr][itm]);             
        }else
          lcd.print(Screens[scr][itm]);     
                 
        if (itm==0) lcd.drawFastHLine(0,7,90,BLACK); //Линия
   

        
        /*Настройки - Подсветка/Звук*/
        if (scr == 3 && (itm==1||itm==2)) {
          if (enter==1){
            backlight = (item==1)?!backlight:backlight; 
            buzzer = (item==2)?!buzzer:buzzer;              
            enter=0;
            EEPROM.write(item,(item==1)?backlight:buzzer);
            EEPROM.write(0,210);
          }
          lcd.print((itm==1?backlight:buzzer)?Screens[4][0]:Screens[4][1]); 
        }
        analogWrite(BACKLIGTH,backlight?200:0);
      }
      d++;
    }
  }
  lcd.display();
}



void onBtnCenterClick() {
  for (int r =0; r < 8; r++){
    if (screen == Routes[r][0] && item == Routes[r][1]){
      lastScreen = screen;
      screen = Routes[r][2];
      enter =  Routes[r][3];
      if (lastScreen != screen) item = 1;
      break;
    }  
  }  
  beep(50);
  timer = millis();
  lcdDrawScreen();
}
void onBtnDownClick(){
  if (screen==2) {
    if (parktronikAction == 1) metr = (metr==0)?0:metr-1; 
    if (parktronikAction == 2) cmetr = (cmetr==0)?0:cmetr-1;
  }else {
    if (!singleClick) return;
    enter = 0;
    item = (item==3)?1:item+1;
    beep(50);     
  }
  timer = millis();
  lcdDrawScreen();
}
void onBtnUpClick(){
  if (screen==2) {
    if (parktronikAction == 1) metr = (metr==5)?5:metr+1;
    if (parktronikAction == 2) cmetr = (cmetr==99)?99:cmetr+1;
  }else {
    if (!singleClick) return;
    enter = 0;
    item = (item==1)?3:item-1;
    beep(50);      
  }
  timer = millis();
  lcdDrawScreen();
}

void onDubleBtnClick(){  
  singleClick = true;
  screen  = 0;
  enter = 0;
  beep(50);
  beep(50);
  parktronikAction = 1;
  partronik = false;
  timer = millis();
  lcdDrawScreen();  
}


void setup() {
  lcdShowLogo();
  if (!lowBattery(readVcc())){
    timer = millis();
    lastDist = 0;
    currentDist = 0;
    screen = 0;
    item = 1;
    enter= 0;
    metr = 0;
    cmetr = 15;
    /*Перебор массива с русскими названиям, для корректного отображения при дальнейшей работе*/
    for (int scr = 0; scr < 5; scr++) {    
      for (int itm = 0; itm <4; itm++) {
        Screens[scr][itm] = utf8rus(Screens[scr][itm]);
      }
    }  
    Serial.begin(9600);
    
    // настройка пинов
    pinMode(sensVCC, OUTPUT);
    pinMode(BuzzerPIN, OUTPUT);
    
    //** Чтение настроек из EEPROM**//
    if (EEPROM.read(0) == 210) {      // если было сохранение настроек, то восстанавливаем их
      backlight = EEPROM.read(1);
      buzzer = EEPROM.read(2);
    }
    if (backlight) analogWrite(BACKLIGTH, 1023);
    //lcdShowLogo(); // Запускаем LCD
    playMelody();
    
    /*Buttons************/
    btnUp.setDebounce(10);        // настройка антидребезга (по умолчанию 80 мс)
    btnUp.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)
    btnUp.setClickTimeout(100);
    btnUp.setType(LOW_PULL);
    btnUp.setDirection(NORM_OPEN);  
    
    btnDown.setDebounce(10);        // настройка антидребезга (по умолчанию 80 мс)
    btnDown.setTimeout(300);        // настройка таймаута на удержание (по умолчанию 500 мс)
    btnDown.setClickTimeout(100);
    btnDown.setType(LOW_PULL);
    btnDown.setDirection(NORM_OPEN);  
    
    /*btnCenter.setDebounce(10);        // настройка антидребезга (по умолчанию 80 мс)
    btnCenter.setTimeout(500);        // настройка таймаута на удержание (по умолчанию 500 мс)
    btnCenter.setClickTimeout(100);*/
    btnCenter.setType(LOW_PULL);
    btnCenter.setDirection(NORM_OPEN);  
    lcdDrawScreen();
  }
}

void loop() {  
  if (!lowBattery(readVcc())){
    //drawBatteryState(readVcc());    
    
    btnUp.tick();
    btnDown.tick();
    btnCenter.tick();
    
    if (btnUp.isSingle())  onBtnUpClick();
    if (btnDown.isSingle())  onBtnDownClick();
    if (btnCenter.isSingle())  onBtnCenterClick();
    if (screen!=2 ){
      if (btnUp.isHolded()) onDubleBtnClick();  
      if (btnDown.isHolded()) onDubleBtnClick();  
    }else 
    if (screen==2 ) {
      if (parktronikAction < 3) {
        if (btnUp.isHold())  onBtnUpClick();
        if (btnDown.isHold())  onBtnDownClick();
      }
      if (parktronikAction == 3) {
        if (btnUp.isHolded()) {onDubleBtnClick();}  
        if (btnDown.isHolded()) {onDubleBtnClick();}  
      }
      getParktronik ();
      lcdDrawScreen();  
    }
    powerSave();
  }else {
    /* https://circuitdigest.com/microcontroller-projects/arduino-sleep-modes-and-how-to-use-them-to-reduce-power-consumption  */
    LowPower.idle(SLEEP_FOREVER, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF,
                 SPI_OFF, USART0_OFF, TWI_OFF);
  }
}

float getSonar(byte i) {  
  dist_3[i] = (float)sonar.ping() / 57.5;                 // получить расстояние в текущую ячейку массива
  if (roulette) dist_3[i] += case_offset;                 // если рулетка, прибавить case_offset
  dist = middle_of_3(dist_3[0], dist_3[1], dist_3[2]);    // фильтровать медианным фильтром из 3ёх последних измерений
  delta = abs(dist_filtered - dist);                      // расчёт изменения с предыдущим
  if (delta > 1) k = 0.7;                                 // если большое - резкий коэффициент
  else k = 0.1; 
  // если маленькое - плавный коэффициент
  dist_filtered = dist * k + dist_filtered * (1 - k);     // фильтр "бегущее среднее"
  return dist_filtered;
}

float getDistance (){
  for (int s = 0; s <= 39; s++) {  
    if (i > 1) i = 0;
    else i++;    
    dist_filtered = getSonar(i);
    //delay(20);
  }
  currentDist = dist_filtered;
  beep(150);
  return dist_filtered;  
}

float getParktronik (){
  if (millis() - sensTimer > 50) {   // измерение и вывод каждые 50 мс
    // счётчик от 0 до 2
    // каждую итерацию таймера i последовательно принимает значения 0, 1, 2, и так по кругу
    if (i > 1) i = 0;
    else i++;
    partronikDist = getSonar(i);
    sensTimer = millis();
  }
    
}
