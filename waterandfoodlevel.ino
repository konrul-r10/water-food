//el psy congroo


#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <NTPClient.h>


// wifi
const char* ssid = "XXXXXXXXXXXXX"; //wifi bilgisi
const char* password = "XXXXXXXXXXXXX";
WiFiClient espClient;

// wifi UDP for NTP, :)
WiFiUDP ntpUDP;
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "3.tr.pool.ntp.org"
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

// OTA
#define SENSORNAME "Vega" //cihazin adi
#define OTApassword "XXX" //OTA baglanti sifresi
int OTAport = 8266;

// MQTT
const char* mqtt_server = "XXX.XXX.XXX.XXX"; // MQTT IP adresi
const char* mqtt_username = ""; //
const char* mqtt_password = "";
const int mqtt_port = XXXX; //Port
PubSubClient client(espClient);
// MQTT TOPICS  
const char* lastfed_topic = "home/catfeeder/lastfed"; // UTF zaman
const char* remaining_topic = "home/catfeeder/remaining"; //kalan mama miktari
const char* willTopic = "home/catfeeder/LWT";  // durum raporu
const char* remaining_topicc = "home/catwater/remaining"; //kalan su miktari
const char* feed_topicc = "home/catfeeder/besle";  // emir tusu

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ultrasonic
long t;
int trigger = D8;
int echo = D7;
float distance;
float percentageFood;
float max_food = 23;  // mama kabi boyu (16,55 iyidir.)48

// Button
const int buttonPin = 3;     // acil durum tusu
////////////////su sensoru/////////////////
/* Change these values based on your calibration values */
#define soilWet 565   // Define max value we consider soil 'wet'
#define soilDry 173   // Define min value we consider soil 'dry'

// Sensor pins
#define sensorPower D1
#define sensorPin A0


void setup() {
  pinMode(sensorPower, OUTPUT);
  
  // Initially keep the sensor OFF
  digitalWrite(sensorPower, LOW);
  // Serial setup
  Serial.begin(9600);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  pinMode(trigger, OUTPUT);
  pinMode(echo, INPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LEDs  pin as an output
  pinMode(2, OUTPUT); // ^ other led
    
  pinMode(buttonPin, INPUT_PULLUP);  // initialize the pushbutton pin as an input:
  
  // Wifi connection setup
  setup_wifi();
  timeClient.begin();
  timeClient.setTimeOffset(10800);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Turn OFF builtin leds
  digitalWrite(BUILTIN_LED, LOW);
  digitalWrite(2, LOW);
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // OTA setup
  ArduinoOTA.setHostname(SENSORNAME);
  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setPassword((const char *)OTApassword);
  ArduinoOTA.begin();
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Baglaniyor ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP); // bit more power hungry, but seems stable.
  WiFi.hostname("VegaMama"); // This will (probably) happear on your router somewhere.
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi baglandi");
  Serial.println("IP adresi: ");
  Serial.println(WiFi.localIP());
}
/********************************** START CALLBACK*****************************************/
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mesaj iletildi [");
  Serial.print(topic);
  Serial.print("] ");
  
  char message[length];
  for (int i = 0; i < length; i++) {
    message[i] = ((char)payload[i]);
    
  }
  message[length] = '\0';
  Serial.print("[");
  Serial.print(message);
  Serial.print("]");
  Serial.println();


}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

 
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void feedCats() {

    // Turn OFF builtin leds
  digitalWrite(BUILTIN_LED, LOW);
  digitalWrite(2, LOW);

  
  timeClient.update();   // arada calismiyor
  String formattedTime = timeClient.getFormattedDate();
  char charBuf[20];
  formattedTime.toCharArray(charBuf, 20);
  client.publish(lastfed_topic, charBuf, true); // Publishing time of feeding to MQTT Sensor retain true
  Serial.print("Mama verme zamani: ");
  Serial.print(charBuf);
  Serial.println();
  digitalWrite(2, LOW); // Turn off onboard LED
  calcRemainingFood();
  delay(300);
}


// calc remaining food in %
void calcRemainingFood() {
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  t = (pulseIn(echo, HIGH) / 2);
  if (t == 0.00) {
    Serial.println("SR02 sensoru okunamadi");
    delay(1000);
    return;
  }
  distance = float(t * 0.0343);
  //Serial.println(distance);
  //Serial.println(t);
  percentageFood = (100 - ((100 / max_food) * distance));
  if (percentageFood < 0.00) {
    percentageFood = 0.00;
  }
  Serial.print("Kalan mama:\t");
  Serial.print(percentageFood);
  Serial.println(" %");
  char charBuf[6];
  int ret = snprintf(charBuf, sizeof charBuf, "%f", percentageFood);  // Translate float to char before publishing...
  client.publish(remaining_topic, charBuf, true); // Publishing remaining food to MQTT Sensor retain true
  delay(1000);
}

void reconnect() {
  // Loop until we're reconnected, i may wanna check for pushbutton here somewhere in case of wifi disaster?
  while (!client.connected()) {
    Serial.print("MQTT baglantisi deneniyor...");
    // Attempt to connect
    if (client.connect(SENSORNAME, mqtt_username, mqtt_password, willTopic, 1, true, "Sistem Uykuda")) {
      client.publish(willTopic,"Uygun", true);
      Serial.println("baglandi");
      // ... and resubscribe
    } else {
      Serial.print("MQTT basarisiz, rc=");
      Serial.print(client.state());
      Serial.println(" 5 saniye icinde tekrar deneyin");
      // Wait 5 seconds before retrying
      delay(500);
    }
  }
}

void loop() {
   
  ArduinoOTA.handle();
  // Check for buttonpin push
  if (digitalRead(buttonPin) == LOW) {       
    Serial.println("Tusa basildi mama veriliyor...");
    digitalWrite(BUILTIN_LED, LOW);
    
    feedCats();
  }
  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  delay(100);
  // Mesafe ölçümünü her 10 dakikada bir yapmak için 10 dakika bekleniyor
  delay(1000);
  digitalWrite(trigger, LOW);
  delayMicroseconds(2);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  t = (pulseIn(echo, HIGH) / 2);
  if (t == 0.00) {
    Serial.println("SR02 sensoru okunamadi");
    delay(1000);
    return;
  }
  distance = float(t * 0.0343);
  //Serial.println(distance);
  //Serial.println(t);
  percentageFood = (100 - ((100 / max_food) * distance));
  if (percentageFood < 0.00) {
    percentageFood = 0.00;
  }
  Serial.print("Kalan mama:\t");
  Serial.print(percentageFood);
  Serial.println(" %");
  char charBuf[6];
  int ret = snprintf(charBuf, sizeof charBuf, "%f", percentageFood);  // Translate float to char before publishing...
  client.publish(remaining_topic, charBuf, true); // Publishing remaining food to MQTT Sensor retain true
  delay(1000);
  mesa(); 
}
void mesa() {

  //get the reading from the function below and print it
  int moisture = readSensor();
  int mappedValue = map(moisture, 0, 1023, 1023, 0);
  Serial.print("Analog Output: ");
  Serial.println(mappedValue);

  // Determine status of our soil
  if (mappedValue > soilWet) {
    String message = String(mappedValue);
    client.publish(remaining_topicc, message.c_str(), true);
    Serial.println("Su tamamen dolu");
  } else if (mappedValue > soilDry && mappedValue <= soilWet) {
    String message = String(mappedValue);
    client.publish(remaining_topicc, message.c_str(), true);
    Serial.println("Su yeterli");
  } else {
    String message = String(mappedValue);
    client.publish(remaining_topicc, message.c_str(), true);
    Serial.println("Su bitti!");
  }
  
  delay(1000);  // Take a reading every second for testing
          // Normally you should take reading perhaps once or twice a day
  Serial.println();
}

//  This function returns the analog soil moisture measurement
int readSensor() {
  digitalWrite(sensorPower, HIGH);  // Turn the sensor ON
  delay(10);              // Allow power to settle
  int val = analogRead(sensorPin);  // Read the analog value form sensor
  digitalWrite(sensorPower, LOW);   // Turn the sensor OFF
  return val;             // Return analog moisture value
}
