/**
* Альтернативная прошивка для управления модулем SONOFF LED
* https://www.itead.cc/sonoff-led.html
* Версия 1.0
* 
При недоступности WiFi подключения старт в режиме точки дступа
Авторизованный доступ к странице настроек. Пароль к WEB по умолчанию "admin". Настройки сохраняются в EEPROM
Обновление прошивки через WEB
Установка уровня яркости по обоим каналам (теплый и холодный свет) 0-100% с шагом 1%
Сохранение настроек сети и диммера в энергонезависимую память
Управление по HTTP GET для интеграции с устройствами умного дома http://<адрес>/on?auth=<pass> и http://<адрес>/off?auth=<pass>
Пароль для GET HTTP устанавливается в настройках. По умолчанию - "12345"
Пароли сохраняются в EEPROM в зашифрованном виде SHA256
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include <arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <ESP8266HTTPUpdateServer.h>
//#include <Ticker.h>
#include "power.h"

#include "WC_EEPROM.h"
#include "WC_HTTP.h"
#include "SHA256.h"
#include "sav_button.h"

// Перегружаться, если в точке доступа
#define AP_REBOOT_TM 600

bool setup_debug = true;

uint32_t ms, ms1 = 0,ms2 = 0, ms_ok = 0;

uint8_t  blink_loop  = 0;
uint8_t  modes_count = 0; 
uint8_t  blink_mode  = 0B00000101;

bool relay_on   = true;

uint8_t PIN_SEL       = 5;
uint8_t PIN_CF        = 14;
uint8_t PIN_CF1       = 13;
uint8_t PIN_RELAY     = 12;
uint8_t PIN_LED       = 15;
uint8_t PIN_BUTTON    = 0;



SButton b1(PIN_BUTTON ,100,5000,0,0,0); 

ESP8266PowerClass power_dev;

uint8_t INT_PIN  = 255;

bool stat1       = true;
bool stat2       = true;
bool sel_stat    = true;
bool cf_stat     = false;
bool reset_flag  = false;
uint16_t count1  = 0;
uint16_t count2  = 0;
uint16_t val_cf  = 0;
uint16_t val_cf1 = 0;
uint16_t val_cf2 = 0;
uint16_t val2_cf  = 0;
uint16_t val2_cf1 = 0;
uint16_t val2_cf2 = 0;

uint32_t stat_ms      = 0;
uint32_t stat_ms_beg  = 0;
uint32_t stat_ms_avg  = 0;
bool     stat_flag    = false;
bool     stat_complete = false;
uint16_t stat_count   = 0;

#define SIZE 720
#define P_DELAY 20000
double P_mas[SIZE];
double P_sum = 0, P_cur = 0;
int    P_count = 0;
uint32_t P_on = 0, P_off = 0;


//Ticker timer;

/*
 
void timerIsr(){
   bool cf  = digitalRead(PIN_CF);
   bool cf1 = digitalRead(PIN_CF1);
   if( cf && cf!=stat1 )count1++;
   stat1 = cf;
   if( cf1 && cf1!=stat2 )count2++;
   stat2 = cf1;
}
*/
/*
void cfInterrupt1(){
 
   uint32_t ms0 = millis();
   if( (ms0 - stat_ms_beg ) >= 1000 ){
      stat_complete = true;    
      return;
   }
//   stat_ms = ms0; 
  
   count1++;
}

void cfInterrupt2(){
   count2++;
}
*/

void setup() {
   pinMode(PIN_RELAY,OUTPUT);
   digitalWrite(PIN_RELAY,HIGH);

// Последовательный порт для отладки
   DebugSerial.begin(115200);
   DebugSerial.printf("ESP8266 Controller. %s\n",NetConfig.ESP_NAME);
// Инициализация EEPROM
   EC_begin();  
   EC_read();
//   pinMode(PIN_CF,INPUT);
//   pinMode(PIN_CF1,INPUT);
   
   
   pinMode(PIN_LED, OUTPUT);
   digitalWrite(PIN_LED,HIGH);
   delay(500);
   digitalWrite(PIN_LED,LOW);
   b1.begin();  
   pinMode(PIN_SEL, OUTPUT);
   digitalWrite(PIN_SEL,sel_stat);
   

// Подключаемся к WiFi  
   WiFi_begin();
  if( isConnect )blink_mode = 0B00000000;
  else blink_mode = 0B11111010;
   
   delay(2000);
// Старт внутреннего WEB-сервера  
   HTTP_begin(); 
   power_dev.enableMeasurePower();
   power_dev.selectMeasureCurrentOrVoltage(CURRENT);
   power_dev.startMeasure();
    
//   timer.attach(0.001, timerIsr);
//    ResetStat();
//    InterruptON(PIN_CF1);
//    attachInterrupt(PIN_CF,  cfInterrupt1, RISING);
//    attachInterrupt(PIN_CF1, cfInterrupt2, RISING);
     
}


int lev1=0;
int lev2=0;



void loop() {
   ms = millis();
// Счетчик импульсов грубо


//   t_cur       = ms/1000;
   switch( b1.Loop() ){
      case  SB_CLICK :
         relay_on = !relay_on;
         digitalWrite(PIN_RELAY,relay_on);
         break;
      case  SB_LONG_CLICK :
         isAP = !isAP;
         if( isAP )blink_mode = 0B00010001;
         WiFi_begin();
         break;
   }

// Пишем профиль мощности   
   if( ( ms - ms2 ) > P_DELAY|| ms < ms2  ){ 
       ms2 = ms;
       if( P_count < SIZE ){
          P_cur = power_dev.getPower();
          P_mas[P_count++] = P_cur;
          P_sum += P_cur;          
       }
   }
   
/*
// Считываем счетчик каждую секунду  
   if( ( ms - ms2 ) > 250|| ms < ms2  ){

    
       ms2 = ms;
       if( reset_flag )ResetStat();
       if( stat_complete ){
          if( cf_stat ){
//             cf_stat = !cf_stat;
             InterruptOFF();
              val_cf  = count1;   
             ResetStat();
             InterruptON(PIN_CF);
             reset_flag = true;
          }
          else {
//             cf_stat = !cf_stat;
             InterruptOFF();
             if( sel_stat ){
                val_cf1 = count1;
             }
             else {
                val_cf2 = count1;
             }
             sel_stat = !sel_stat;
             digitalWrite(PIN_SEL, sel_stat);
             ResetStat();
             InterruptON(PIN_CF1);
             reset_flag = true;
          }
 //         count1  = 0;
       }
    }
*/

// Событие срабатывающее каждые 125 мс   
   if( ( ms - ms1 ) > 125|| ms < ms1 ){
       ms1 = ms;
// Режим светодиода ищем по битовой маске       
       if(  blink_mode & 1<<(blink_loop&0x07) ) digitalWrite(PIN_LED, LOW); 
       else  digitalWrite(PIN_LED, HIGH);
       blink_loop++;    
    }


   
   if( isAP && ms > AP_REBOOT_TM*1000 ){
       DebugSerial.printf("TIMEOUT. REBOOT. \n");
       ESP.reset();
   }
   HTTP_loop();

}
/*
void InterruptON(uint8_t pin){
   INT_PIN = pin;
   attachInterrupt(INT_PIN,  cfInterrupt1, RISING);   
}

void InterruptOFF(){
   detachInterrupt(INT_PIN);   
}


void ResetStat(){
   stat_ms_beg  = millis();
   stat_ms      = stat_ms_beg;
   stat_ms_avg  = 0;
   stat_flag    = false;
   stat_complete = false;
   stat_count   = 0;  
   count1       = 0;
   reset_flag   = false;
}
*/



