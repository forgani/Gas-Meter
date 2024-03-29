 /*******************************************************************************
 
  ESP-12 based   
  Gas meter counter with reed switch for measurement
  Using ESP8266 RTC built-in timer and implement a light sleep mode on an esp8266 (ESP-12E). 
  Each time the reed switch closes, the ESP wakes up from light sleep and we count the reboots.
  
  Mohammad Forgani, Forghanain
  https://www.forgani.com/electronics-projects/gas-meter-counter-with-reed-switch-for-measurement/
  or
  https://github.com/forgani/Gas-Meter
  
  The concept is to let ESP8266 to Light Sleep indefinitely, only to be waken up when the level changes from HIGH to LOW 
  on GPIO02 pin. 
  
  initial version 10 Dec. 2021
  update on 08 Nov. 2022 fixed for Arduino 2.0
  Generic esp8266
******************************************************************************/
#include "Arduino.h"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <WidgetRTC.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Required for LIGHT_SLEEP_T delay mode
extern "C" {
  #include "user_interface.h"
}
// Sleep for longest possible time
#define FPM_SLEEP_MAX_TIME    0xFFFFFFF
#define LIGHT_WAKE_PIN        2     	//GPIO2  

double kWh_factor=10.7, calorific_value=6.65;  // cent

/*---------- SYNC ALL SETTINGS ON BOOT UP ----------*/
bool Connected2Blynk = false;
uint32_t _counter = 0;
uint32_t _lastDay = 0;
uint32_t _today = 0;
uint32_t cnt = 0;
bool tryToConnect = true;

//----------------BLYNK Virtual Pins -----
#define vPIN_CNT  				V91
#define vPIN_DATE 				V92
#define vPIN_kWh_24h          			V93
#define vPIN_bill_24h         			V94
#define vPIN_CALIBRATION_calorific_value  	V95
#define vPIN_CALIBRATION_kWh_factor       	V96
#define vPIN_voltage       			V97
#define vPIN_CALIBRATION_bat 			V98

#define BLYNK_PRINT Serial 

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

//
int analogInPin = A0;        // Analog channel A0 as used to measure battery voltage
/*
Vin = 4.2V      Max Input Voltage
R1 = 15k
R2 = 50k
Vout max = 1.0V  by 1023bit
Vout = (R2/(R1+R2))*Vin => 0.969V  is Okay
Resistors Ration Factor = Max Input Voltage/1023 => 4.2V/1023bit = 0.0041
*/
float calibration = 0.0041; // Resistors Ration Factor
//

//B L Y N K    C O N N E C T I O N
char auth[] = "xxx"; 
char ssid[] = "xxx"; 
char pass[] = "xxx"; 
IPAddress BlynkServerIP(xxx, xxx, xxx, xxx);
int port = xxx;

//The setup function is called once at startup of the sketch
void setup() {
  Serial.begin(115200);
  //while(!Serial) { }
	
  Serial.println();
  gpio_init(); // Initilise GPIO pins
	
  Serial.println("Start device in normal mode!");
  pinMode (LIGHT_WAKE_PIN, INPUT);
  Blynk.virtualWrite(vPIN_CALIBRATION_calorific_value, calorific_value);
  Blynk.virtualWrite(vPIN_CALIBRATION_kWh_factor, kWh_factor);
  
  //...
  // Force the ESP into client-only mode 
  WiFi.mode(WIFI_STA); 
  // Enable light sleep 
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  Serial.println("setup done");
}

BLYNK_WRITE(vPIN_CALIBRATION_calorific_value) {// calibration slider 
  calorific_value = param.asDouble();
}

BLYNK_WRITE(vPIN_CALIBRATION_kWh_factor) {// calibration slider
  kWh_factor = param.asDouble();
}

void sleepNow(){
  Serial.println("Enter light sleep mode");
  wifi_station_disconnect();
  wifi_set_opmode_current(NULL_MODE);
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open(); // Enables force sleep
  gpio_pin_wakeup_enable(GPIO_ID_PIN(LIGHT_WAKE_PIN), GPIO_PIN_INTR_LOLEVEL);  //   GPIO_PIN_INTR_NEGEDGE
  delay(500);
  wifi_fpm_set_wakeup_cb(wakeupFromMotion);
  wifi_fpm_do_sleep(FPM_SLEEP_MAX_TIME); // Sleep for longest possible time
  delay(500);
}

void wakeupFromMotion(void) {
  wifi_set_sleep_type(NONE_SLEEP_T);
  gpio_pin_wakeup_disable();
  wifi_fpm_close();
  wifi_set_opmode(STATION_MODE);
  wifi_station_connect();
  Serial.println("Woke up from sleep");
}

void initWifi() {
  Serial.println("Connecting to wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  int cnt = 0;
  int tryToConnect = true;
  while (tryToConnect) {
	  delay(1000);
	  Serial.print(".");
    cnt++;
    if (cnt >= 10 || WiFi.status() == WL_CONNECTED) tryToConnect = false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wifi connection failed!");
  } else {
	  Serial.println("WiFi connected."); Serial.print("IP address:"); Serial.println(WiFi.localIP());
    Blynk.config(auth, BlynkServerIP, port);
    Blynk.connect(); 
    
    if (Blynk.connected() == false) {
      Serial.println("Not connected to Blynk server");
      Blynk.connect(); // try to connect to server with default timeout
    } 
    
    if (Blynk.connected() == true){
      Serial.println("Connected to Blynk server");
      Blynk.run();
    } 
    
    timeClient.begin();
  }
  timeClient.update();
} 

 
void loop() {
    sleepNow();
    Serial.println("Exit light sleep mode");
    initWifi();

    Blynk.virtualWrite(vPIN_DATE, timeClient.getFormattedTime());
    Serial.println("Date: " + String(timeClient.getFormattedTime()));
    ESP.rtcUserMemoryRead(0, &_counter, sizeof(_counter));
    Serial.println("Last meterCount: " + String(_counter));
    _counter++;
    
    ESP.rtcUserMemoryRead(sizeof(_counter), &_lastDay, sizeof(_lastDay));
    _today = timeClient.getDay();
    Serial.println("LastDay: " + String(_lastDay) + "  Today: " + String(_today));
    
    if (_lastDay != _today) {
        double kWh_24h = kWh_factor * (_counter/10) * 24;  // 1 pulse -> 100dm³
        Blynk.virtualWrite(vPIN_kWh_24h, kWh_24h);
        double kWh_daily_bill = (kWh_24h * calorific_value)/100; // €
        Blynk.virtualWrite(vPIN_bill_24h, kWh_daily_bill);
    
        _counter=1;
        _lastDay = _today;
        ESP.rtcUserMemoryWrite(sizeof(_counter), &_lastDay, sizeof(_lastDay));
    }
    
    ESP.rtcUserMemoryWrite(0, &_counter, sizeof(_counter));
    Serial.println("Current meterCount: " + String(_counter));
	  Blynk.virtualWrite(vPIN_CNT, String(_counter));

    wifi_station_disconnect();
    wifi_set_opmode_current(NULL_MODE);
    wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
    wifi_fpm_open(); // Enables force sleep

    while(digitalRead(LIGHT_WAKE_PIN) == LOW) {
      delay(500);
    } 
    delay(100);
} 

// This function will run every time Blynk connection is established
BLYNK_CONNECTED() {
  Blynk.syncAll();
}
