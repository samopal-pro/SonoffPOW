/**
* Реализация HTTP сервера
* 
* Copyright (C) 2016 Алексей Шихарбеев
* http://samopal.pro
*/
#include "WC_HTTP.h"
#include "WC_EEPROM.h"
#include "logo.h"

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

bool isAP = false;
bool isConnect  = false;

String authPass = "";
String HTTP_User = "";
String WiFi_List;
int    UID       = -1;




/**
 * Старт WiFi
 */
void WiFi_begin(void){ 
// Определяем список сетей
  ListWiFi(WiFi_List);
// Подключаемся к WiFi

  isAP = false;
  if( ! ConnectWiFi()  ){
      if( setup_debug )DebugSerial.printf("Start AP %s\n",NetConfig.ESP_NAME);
      WiFi.mode(WIFI_STA);
      WiFi.softAP(NetConfig.ESP_NAME);
      isAP = true;
      if( setup_debug )DebugSerial.printf("Open http://192.168.4.1 in your browser\n");
      isConnect = false; 
 }
  else {
// Получаем статический IP если нужно  
      if( NetConfig.IP != 0 ){

         WiFi.config(NetConfig.IP,NetConfig.GW,NetConfig.MASK);
         if( setup_debug )DebugSerial.print("Open http://");
         if( setup_debug )DebugSerial.print(WiFi.localIP());
         if( setup_debug )DebugSerial.println(" in your browser");
      }
   }
// Запускаем MDNS
    MDNS.begin(NetConfig.ESP_NAME);
    if( setup_debug )DebugSerial.printf("Or by name: http://%s.local\n",NetConfig.ESP_NAME);
    isConnect = true; 
    

   
}

/**
 * Соединение с WiFi
 */
bool ConnectWiFi(void) {

  // Если WiFi не сконфигурирован
  if ( strcmp(NetConfig.AP_SSID, "none")==0 ) {
     if( setup_debug )DebugSerial.printf("WiFi is not config ...\n");
     return false;
  }

  WiFi.mode(WIFI_STA);

  // Пытаемся соединиться с точкой доступа
  if( setup_debug )DebugSerial.printf("\nConnecting to: %s/%s\n", NetConfig.AP_SSID, NetConfig.AP_PASS);
  WiFi.begin(NetConfig.AP_SSID, NetConfig.AP_PASS);
  delay(1000);

  // Максиммум N раз проверка соединения (12 секунд)
  for ( int j = 0; j < 15; j++ ) {
  if (WiFi.status() == WL_CONNECTED) {
      if( setup_debug )DebugSerial.print("\nWiFi connect: ");
      if( setup_debug )DebugSerial.print(WiFi.localIP());
      if( setup_debug )DebugSerial.print("/");
      if( setup_debug )DebugSerial.print(WiFi.subnetMask());
      if( setup_debug )DebugSerial.print("/");
      if( setup_debug )DebugSerial.println(WiFi.gatewayIP());
      return true;
    }
    delay(1000);
    if( setup_debug )DebugSerial.print(WiFi.status());
  }
  if( setup_debug )DebugSerial.printf("\nConnect WiFi failed ...\n");
  return false;
}

/**
 * Найти список WiFi сетей
 */
void ListWiFi(String &out){
  int n = WiFi.scanNetworks();
  if( n == 0 )out += "<p>Сетей WiFi не найдено";
  else {
     out = "<select name=\"AP_SSID\">\n";
     for (int i=0; i<n; i++){
       out += "    <option value=\"";
       out += WiFi.SSID(i);
       out += "\"";
       if( strcmp(WiFi.SSID(i).c_str(),NetConfig.AP_SSID) == 0 )out+=" selected";
       out += ">";
       out += WiFi.SSID(i);
       out += " [";
       out += WiFi.RSSI(i);
       out += "dB] ";
       out += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
       out += "</option>\n";
     }     
     out += "</select>\n";
  }
}

 

/**
 * Старт WEB сервера
 */
void HTTP_begin(void){
// Поднимаем WEB-сервер  
   server.on ( "/", HTTP_handleRoot );
   server.on ( "/config", HTTP_handleConfig );
   server.on ( "/default", HTTP_handleDefault );
   server.on ( "/login", HTTP_handleLogin );
   server.on ( "/logo", HTTP_handleLogo );

   server.on ( "/reboot", HTTP_handleReboot );
   server.on ( "/graph", HTTP_handleGraph );
   server.on ( "/data", HTTP_handleData );

   server.onNotFound ( HTTP_handleRoot );
  //here the list of headers to be recorded
   const char * headerkeys[] = {"User-Agent","Cookie"} ;
   size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  //ask server to track these headers
   server.collectHeaders(headerkeys, headerkeyssize );
   httpUpdater.setup(&server,"/update");


   
   server.begin();
   if( setup_debug )DebugSerial.printf( "HTTP server started ...\n" );
  
}


/**
 * Вывод в буфер одного поля формы
 */
void HTTP_printInput(String &out,const char *label, const char *name, const char *value, int size, int len, bool is_pass){
   char str[10];
   if( strlen( label ) > 0 ){
      out += "<td>";
      out += label;
      out += "</td>\n";
   }
   out += "<td><input name ='";
   out += name;
   out += "' value='";
   out += value;
   out += "' size=";
   sprintf(str,"%d",size);  
   out += str;
   out += " length=";    
   sprintf(str,"%d",len);  
   out += str;
   if( is_pass )out += " type='password'";
   out += "></td>\n";  
}

/**
 * Выаод заголовка файла HTML
 */
void HTTP_printHeader(String &out,const char *title, uint16_t refresh){
  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n";
  if( refresh ){
     char str[10];
     sprintf(str,"%d",refresh);
     out += "<meta http-equiv='refresh' content='";
     out +=str;
     out +="'/>\n"; 
  }
  out += "<title>";
  out += title;
  out += "</title>\n";
  out += "<style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n</head>\n";
  out += "<body>\n";
  out += "<img src=/logo>\n";
  out += "<p><b>Устройство: ";
  out += NetConfig.ESP_NAME;
  if( UID < 0 )out +=" <a href=\"/login\">Авторизация</a>\n";
  else out +=" <a href=\"/login?DISCONNECT=YES\">Выход</a>\n";
  out += "</b></p>";
  out += title;
  out += "</h1>\n";
}   
 
/**
 * Выаод окнчания файла HTML
 */
void HTTP_printTail(String &out){
  out += "</body>\n</html>\n";
}

/**
 * Ввод имени и пароля
 */
void HTTP_handleLogin(){
  String msg;
// Считываем куки  
  if (server.hasHeader("Cookie")){   
//    DebugSerial.print("Found cookie: ");
    String cookie = server.header("Cookie");
//    DebugSerial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")){
    if( setup_debug )DebugSerial.println("Disconnect");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESP_PASS=\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  if ( server.hasArg("PASSWORD") ){
    String pass = server.arg("PASSWORD");
    
    if ( HTTP_checkAuth((char *)pass.c_str()) >=0 ){
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESP_PASS="+pass+"\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      if( setup_debug )DebugSerial.println("Login Success");
      return;
    }
  msg = "Неправильный пароль";
  if( setup_debug )DebugSerial.println("Login Failed");
  }
  String out = "";
  HTTP_printHeader(out,"Авторизация");
  out += "<form action='/login' method='POST'>\
    <table border=0 width='600'>\
      <tr>\
        <td width='200'>Введите пароль:</td>\
        <td width='400'><input type='password' name='PASSWORD' placeholder='password' size='32' length='32'></td>\
      </tr>\
      <tr>\
        <td width='200'><input type='submit' name='SUBMIT' value='Ввод'></td>\
        <td width='400'>&nbsp</td>\
      </tr>\
    </table>\
    </form><b>" +msg +"</b><br>";
  HTTP_printTail(out);  
  server.send(200, "text/html", out);
}

/**
 * Обработчик событий WEB-сервера
 */
void HTTP_loop(void){
  server.handleClient();
}

/**
 * Перейти на страничку с авторизацией
 */
void HTTP_gotoLogin(){
  String header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
  server.sendContent(header);
}
/**
 * Отображение логотипа
 */
void HTTP_handleLogo(void) {
  server.send_P(200, PSTR("image/png"), logo, sizeof(logo));
}
  


/*
 * Оработчик главной страницы сервера
 */
void HTTP_handleRoot(void) {
  char str[20];
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 
  
// Включение/отключение реле
   if ( server.hasArg("POWER") ){
       relay_on = !relay_on;
       digitalWrite(PIN_RELAY,relay_on);
       String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
       server.sendContent(header);
       return;
   }
   if ( server.hasArg("PROFILE") ){
       P_sum = 0;
       P_cur = 0;
       P_count = 0;
       String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
       server.sendContent(header);
       return;
   }
// Переключение режима измерения
   if ( server.hasArg("APPLY") ){
       if( strcmp(server.arg("MODE").c_str(),"mode1")==0)cf_stat = true;
       else cf_stat = false;
       String header = "HTTP/1.1 301 OK\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
       server.sendContent(header);
       return;
   }

  
  String out = "";

  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n\
  <title>Sonoff POW</title>\n\
  <style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n\
  <script type=\"text/javascript\" src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js\"></script>\
  <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\
  </head>\n";
  out += "<body>\n";
   if( isAP ){
      out += "<p>Режим точки доступа: ";
      out += NetConfig.ESP_NAME;
   }
   else {
      out += "<p>Подключено к ";
      out += NetConfig.AP_SSID;
   }   
   
   

   out += "<p><a href=\"/config\">Настройки контроллера</a>";

   out +="<form action='/' method='POST'><p>";
   if( relay_on )out +="Реле включено <input type='submit' name='POWER' value='Отключить'>";
   else out +="Реле отключено <input type='submit' name='POWER' value='Включить'>";
   out += "<p>Профиль мощности <input type='submit' name='PROFILE' value='Старт'></form>";
   out += "<p><a href='/graph'>График</a> <a href='/data'>Данные</a>\n";
// Время измерения
   sprintf(str,"%0d:%02d",(P_count*5)/60,(P_count*5)%60);
   out += "<h1>";
   out += str;
   out += "</h1>\n";
   
   out += "<div id=\"chart_div\" style=\"width: 500px; height: 500px\"></div>\n";
   out += "<script type=\"text/javascript\">\n\
      google.charts.load('current', {'packages':['gauge']});\
      google.charts.setOnLoadCallback(drawChart);\
      function drawChart() {\
        var data = google.visualization.arrayToDataTable([\
          ['Label', 'Value'],\
          ['Мощность', 1000],\
          ['Ср. мощность', 1000]\
        ]);\
        var options = {\
          width: 480, height: 240,\
          max: 1000,\
          minorTicks: 5\
        };\    
        var chart = new google.visualization.Gauge(document.getElementById('chart_div'));\
        data.setValue(0, 1,";
        
        double x = power_dev.getPower();
        sprintf(str,"%0d.%02d",(int)x,((int)(x*100))%100);
        out += str;
        out +=");\
        data.setValue(1, 1,";
        
        x = 0;
        if( P_count > 0 )x = P_sum/P_count;
        sprintf(str,"%0d.%02d",(int)x,((int)(x*100))%100);
        out += str;
        out +=");\        
        chart.draw(data, options);\    
      }\
    </script>\n";  
/*
  <div id=\"content\"></div>\n\       
    <script>\  
        function show(){\  
             $.ajax({\  
                url: \"content.htm\",\  
                cache: false,\  
                success: function(html){\  
                    $(\"#content\").html(html);\  
                }\  
            });\  
        }\n\        
        $(document).ready(function(){\  
            show();\  
            setInterval('show()',1000);\  
        });\  
    </script>\n";  
*/
  
//  HTTP_printHeader(out,"Контроллер умного дома v 2.0");
//   

/*   
   out +="<p>Режим измерения: <input type='radio' name='MODE' value='mode1' ";
   if( cf_stat )out+="checked";
   out+=">Потребление э/энергии ";
   out +="<input type='radio' name='MODE' value='mode2'";
   if( !cf_stat )out+="checked";
   out +=">Напряжение/ток/мощность ";
   out +="<input type='submit' name='APPLY' value='Изменить'>";
   out +="</form>";

   out += "<p>CF = ";
   sprintf(str,"%d",val_cf);
   out += str;
   out += "<p>CF1 = ";
   sprintf(str,"%d",val_cf1);
   out += str;
   out += "<p>CF2 = ";
   sprintf(str,"%d",val_cf2);
   out += str;
*/
/*
   double x;
   x = power_dev.getPower();
   sprintf(str,"%0d.%02d",(int)x,((int)(x*100))%100);
   out += "<h2>Мощность";
   out += str;
   out += "</h2>\n";
   x = power_dev.getCurrent();
   sprintf(str,"%0d.%02d",(int)x,((int)(x*100))%100);
   out += "<h2>Ток (А)";
   out += str;
   out += "</h2>\n";
   x = power_dev.getVoltage();
   sprintf(str,"%0d.%02d",(int)x,((int)(x*100))%100);
   out += "<h2>Напряжение, В";
   out += str;
   out += "</h2>\n";
*/   
   HTTP_printTail(out);
   


     
   server.send ( 200, "text/html", out );
}

/*
 * Показать график профиля мощности
 */
void HTTP_handleGraph(void) {
  char str[50];
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 

  
  String out = "";

  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n\
  <title>Sonoff POW</title>\n\
  <style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n\
  <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\
  </head>\n";
   
  

   out += "<p><a href=\"/\">Главная</a>";


   
   out += "<div id=\"chart_div\" style=\"width: 900px; height: 500px\"></div>\n";
   out += "<script type=\"text/javascript\">\n\
      google.charts.load('current', {'packages':['corechart']});\
      google.charts.setOnLoadCallback(drawChart);\
      function drawChart() {\
        var options1 = {\
          title: 'Профиль мощности',\
          curveType: 'function',\
          legend: { position: 'bottom' }\
        };\
        var data1 = google.visualization.arrayToDataTable([\
          ['', 'Вт'],\n";
   for( int i=0; i<P_count; i++){       
       double x = P_mas[i];
       sprintf(str,"[%d,%0d.%02d]",i,(int)x,((int)(x*100))%100);
       out += str;
       if( i < P_count-1 )out += ",\n";
   }
   out += "\n\
         ]);\
    var chart1 = new google.visualization.LineChart(document.getElementById('chart_div'));\
    chart1.draw(data1, options1);\   
    }\
    </script>\n";      
   HTTP_printTail(out);
   


     
   server.send ( 200, "text/html", out );
}  

/*
 * Показать данные профиля мощности
 */
void HTTP_handleData(void) {
  char str[50];
// Проверка авторизации  

  int gid = HTTP_isAuth();
  if ( gid < 0 ){
    HTTP_gotoLogin();
    return;
  } 

  
  String out = "";

  out += "<html>\n<head>\n<meta charset=\"utf-8\" />\n\
  <title>Sonoff POW</title>\n\
  <style>body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }</style>\n\
  </head>\n";
   
  

   out += "<p><a href=\"/\">Главная</a>";


   
   out += "<table border=1>\n";
    for( int i=0; i<P_count; i++){       
       double x = P_mas[i];
       sprintf(str,"<tr><td>%d</td><td>%0d,%02d</td></tr>\n",i,(int)x,((int)(x*100))%100);
       out += str;
   }
   out += "</table>\n";      
   HTTP_printTail(out);
   


     
   server.send ( 200, "text/html", out );
}
 
/*
 * Оработчик страницы настройки сервера
 */
void HTTP_handleConfig(void) {
// Проверка прав администратора  
  char s[65];
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 

// Сохранение контроллера
  if ( server.hasArg("ESP_NAME") ){

     if( server.hasArg("ESP_NAME")     )strcpy(NetConfig.ESP_NAME,server.arg("ESP_NAME").c_str());
     if( server.hasArg("ESP_PASS")     )strcpy(NetConfig.ESP_PASS,server.arg("ESP_PASS").c_str());
     if( server.hasArg("AP_SSID")      )strcpy(NetConfig.AP_SSID,server.arg("AP_SSID").c_str());
     if( server.hasArg("AP_PASS")      )strcpy(NetConfig.AP_PASS,server.arg("AP_PASS").c_str());
     if( server.hasArg("IP1")          )NetConfig.IP[0] = atoi(server.arg("IP1").c_str());
     if( server.hasArg("IP2")          )NetConfig.IP[1] = atoi(server.arg("IP2").c_str());
     if( server.hasArg("IP3")          )NetConfig.IP[2] = atoi(server.arg("IP3").c_str());
     if( server.hasArg("IP4")          )NetConfig.IP[3] = atoi(server.arg("IP4").c_str());
     if( server.hasArg("MASK1")        )NetConfig.MASK[0] = atoi(server.arg("MASK1").c_str());
     if( server.hasArg("MASK2")        )NetConfig.MASK[1] = atoi(server.arg("MASK2").c_str());
     if( server.hasArg("MASK3")        )NetConfig.MASK[2] = atoi(server.arg("MASK3").c_str());
     if( server.hasArg("NASK4")        )NetConfig.MASK[3] = atoi(server.arg("MASK4").c_str());
     if( server.hasArg("GW1")          )NetConfig.GW[0] = atoi(server.arg("GW1").c_str());
     if( server.hasArg("GW2")          )NetConfig.GW[1] = atoi(server.arg("GW2").c_str());
     if( server.hasArg("GW3")          )NetConfig.GW[2] = atoi(server.arg("GW3").c_str());
     if( server.hasArg("GW4")          )NetConfig.GW[3] = atoi(server.arg("GW4").c_str());
     if( server.hasArg("WEB_PASS")   ){
         if( strcmp(server.arg("WEB_PASS").c_str(),"*") != 0 ){
            Sha256.init();
            Sha256.add((char *)server.arg("WEB_PASS").c_str());
            strcpy(NetConfig.WEB_PASS,Sha256.char_result());
         }
     }
     EC_save();
     
     String header = "HTTP/1.1 301 OK\r\nLocation: /config\r\nCache-Control: no-cache\r\n\r\n";
     server.sendContent(header);
     return;
  }

  String out = "";
  char str[10];
  HTTP_printHeader(out,"Настройка контроллера");
  out += "\
     <ul>\
     <li><a href=\"/\">Главная</a>\
     <li><a href=\"/default\">Сброс всех настроек</a>\
     <li><a href=\"/update\">Обновление прошивки</a>\
     <li><a href=\"/reboot\">Перезагрузка</a>\
     </ul>\n";

// Печатаем время в форму для корректировки

// Форма для настройки параметров
   out += "<h2>Конфигурация</h2>";
   out += "<h3>Параметры контроллера в режиме точки доступа</h3>";
   out += "<form action='/config' method='POST'><table><tr>";
   HTTP_printInput(out,"Наименование:","ESP_NAME",NetConfig.ESP_NAME,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Пароль:","ESP_PASS",NetConfig.ESP_PASS,32,32,true);
   out += "</tr></table>";

   out += "<h3>Параметры подключения к WiFi</h3>";
   out += "<table><tr>";
   out += "<td>Сеть WiFi</td><td>"+WiFi_List+"<td>";
//   HTTP_printInput(out,"Сеть WiFi:","AP_SSID",NetConfig.AP_SSID,32,32);
   out += "</tr><tr>";
   HTTP_printInput(out,"Пароль WiFi: &nbsp;&nbsp;&nbsp;","AP_PASS",NetConfig.AP_PASS,32,32,true);
   out += "</tr></table><table><tr>";
   sprintf(str,"%d",NetConfig.IP[0]); 
   HTTP_printInput(out,"Статический IP:","IP1",str,3,3);
   sprintf(str,"%d",NetConfig.IP[1]); 
   HTTP_printInput(out,".","IP2",str,3,3);
   sprintf(str,"%d",NetConfig.IP[2]); 
   HTTP_printInput(out,".","IP3",str,3,3);
   sprintf(str,"%d",NetConfig.IP[3]); 
   HTTP_printInput(out,".","IP4",str,3,3);
   out += "</tr><tr>";
   sprintf(str,"%d",NetConfig.MASK[0]); 
   HTTP_printInput(out,"Маска","MASK1",str,3,3);
   sprintf(str,"%d",NetConfig.MASK[1]); 
   HTTP_printInput(out,".","MASK2",str,3,3);
   sprintf(str,"%d",NetConfig.MASK[2]); 
   HTTP_printInput(out,".","MASK3",str,3,3);
   sprintf(str,"%d",NetConfig.MASK[3]); 
   HTTP_printInput(out,".","MASK4",str,3,3);
   out += "</tr><tr>";
   sprintf(str,"%d",NetConfig.GW[0]); 
   HTTP_printInput(out,"Шлюз","GW1",str,3,3);
   sprintf(str,"%d",NetConfig.GW[1]); 
   HTTP_printInput(out,".","GW2",str,3,3);
   sprintf(str,"%d",NetConfig.GW[2]); 
   HTTP_printInput(out,".","GW3",str,3,3);
   sprintf(str,"%d",NetConfig.GW[3]); 
   HTTP_printInput(out,".","GW4",str,3,3);
   out += "</tr><table>";

   out += "<h3>Доступ к контроллеру</h3>";
   out += "<table><tr>";
   HTTP_printInput(out,"Пароль HTTP:","WEB_PASS","*",32,32,true);
   out += "</tr><table>";
   
   out +="<input type='submit' name='SUBMIT_CONF' value='Сохранить'>"; 
   out += "</form></body></html>";
   server.send ( 200, "text/html", out );
  
}        



/*
 * Сброс настрое по умолчанию
 */
void HTTP_handleDefault(void) {
// Проверка прав администратора  
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 

  EC_default();
  HTTP_handleConfig();  
}



/**
 * Проверка авторизации
 */
int HTTP_isAuth(){
//  DebugSerial.print("AUTH ");
  if (server.hasHeader("Cookie")){   
//    DebugSerial.print("Found cookie: ");
    String cookie = server.header("Cookie");
//    DebugSerial.print(cookie);
 
    if (cookie.indexOf("ESP_PASS=") != -1) {
      authPass = cookie.substring(cookie.indexOf("ESP_PASS=")+9);       
      return HTTP_checkAuth((char *)authPass.c_str());
    }
  }
  return -1;  
}


/**
 * Функция проверки пароля
 * возвращает 0 - админ, 1 - оператор, -1 - Не авторизован
 */
int  HTTP_checkAuth(char *pass){
   char s[65];
// Пароль на доступк к WEB в зашифрованном виде
   Sha256.init();
   Sha256.add(pass);
   strcpy(s,Sha256.char_result());
//   DebugSerial.println(pass);
//   DebugSerial.println(s);
//   DebugSerial.println(NetConfig.WEB_PASS);
   if( strcmp(s,NetConfig.WEB_PASS) == 0 ){ 
       UID = 0;
       HTTP_User = "Администратор";
   }
    else {
       UID = -1;
       HTTP_User = "Анонимус";
   }
//   DebugSerial.printf("Auth is %d\n",UID);
   return UID;
}


/*
 * Перезагрузка часов
 */
void HTTP_handleReboot(void) {
// Проверка прав администратора  
  if ( HTTP_isAuth() != 0 ){
    HTTP_gotoLogin();
    return;
  } 


  String out = "";

  out = 
"<html>\
  <head>\
    <meta charset='utf-8' />\
    <meta http-equiv='refresh' content='10;URL=/'>\
    <title>ESP8266 sensor 1</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Оборудование перезагружается. </h1>\
    </body>\
</html>";
   server.send ( 200, "text/html", out );
   ESP.reset();  
  
}


