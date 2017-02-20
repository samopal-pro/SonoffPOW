/**
* Реализация HTTP сервера
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#ifndef WS_HTTP_h
#define WS_HTTP_h
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include "power.h"


#include <arduino.h>
#include "SHA256.h"
#define DebugSerial Serial


extern ESP8266WebServer server;
extern ESP8266HTTPUpdateServer httpUpdater;
extern ESP8266PowerClass power_dev;
extern bool isAP;
extern bool isConnect;
extern bool relay_on;
extern bool cf_stat;

extern uint8_t PIN_SEL;
extern uint8_t PIN_CF;
extern uint8_t PIN_CF1;
extern uint8_t PIN_RELAY;
extern uint8_t PIN_LED;

extern uint16_t val_cf;
extern uint16_t val_cf1;
extern uint16_t val_cf2;
extern uint16_t val2_cf;
extern uint16_t val2_cf1;
extern uint16_t val2_cf2;

extern double P_mas[];
extern double P_sum, P_cur;
extern int    P_count;
extern uint32_t P_on, P_off;




bool ConnectWiFi(void);
void ListWiFi(String &out);
void HTTP_begin(void);
void HTTP_loop();
void WiFi_begin(void);
void Time2Str(char *s,time_t t);
bool SetParamHTTP();
int  HTTP_isAuth();
void HTTP_handleRoot(void);
void HTTP_handleConfig(void);
void HTTP_handleLogin(void);
void HTTP_handleReboot(void);
void HTTP_handleDefault(void);
void HTTP_handleLogo(void);
void HTTP_gotoLogin();
int  HTTP_checkAuth(char *user);
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len,bool is_pass=false);
void HTTP_printHeader(String &out,const char *title, uint16_t refresh=0);
void HTTP_printTail(String &out);
//void SetPwm();
void HTTP_device(void);
void HTTP_handleGraph(void);
void HTTP_handleData(void);



#endif
