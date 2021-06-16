#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEPROM.h>
// REPLACE WITH YOUR NETWORK CREDENTIALS
const char *ssid = "*****"; // wifi name
const char *pass = "*****"; // wifi password

const char *mqtt_server = "mqtt.by";    // Имя сервера MQTT
const int mqtt_port = 1883;             // Порт для подключения к серверу MQTT
const char *mqtt_user = "***********";       // your login
const char *mqtt_pass = "***********";       // your password

int transistorLeak = 12; // transistor for turn ON leak sensor
int transistorPow = 15;  // transistor for conect power with A0 pin
int leak = 14;           // pin which connected with leak sensor
int power_buttery = A0;  // check buttery power
// some variables
int PowerValueNow;    // buttery value now
int PowerValueBefore; // buttery value in previous session
bool SendFlag = 0;    // send 1 if it's leak or buttery value changed
bool alarm = 1;       // alarm 1 if it's leak
int statistic[10];    // I'll make 10 measurements for statistic
int addrPower = 0;    // plase where i gona keep power value
int temporaryVar;

WiFiClient wclient;
PubSubClient client(wclient, mqtt_server, mqtt_port);
// Function for sending variables to MQTT server
void MQTT_Sending()
{
  // connect to WiFi
  if (WiFi.status() != WL_CONNECTED)
  {

    WiFi.begin(ssid, pass);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
      return;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!client.connected())
    {
      //cjnnect to MQTT server
      if (client.connect(MQTT::Connect("/user/Aleks2312323") // отправка c Esp8266
                             .set_auth(mqtt_user, mqtt_pass)))
      {
      }
    }
    else if (client.connected())
    {
      // Send variables
      client.publish("/leak", String(alarm));
      client.publish("/EnergyLeak", String(PowerValueNow));
      client.publish("/Before", String(PowerValueBefore));
    }
  }
}

void setup()
{
  Serial.setTimeout(2000);
  // adjust pin for output and input
  pinMode(transistorLeak, OUTPUT);
  pinMode(transistorPow, OUTPUT);
  pinMode(leak, INPUT);

  EEPROM.begin(512);
  // compare curren buttery value and previous
  digitalWrite(transistorPow, HIGH);
  // make 10 measurements for statistic
  for (int j = 0; j < 10; j++)
  {
    statistic[j] = analogRead(power_buttery);
  }
  for (int k = 0; k < 10; k++) // sort
  {
    for (int m = 0; m < 10; m++)
    {
      if (statistic[m] > statistic[k])
      {
        temporaryVar = statistic[m];
        statistic[m] = statistic[k];
        statistic[k] = temporaryVar;
      }
    }
  }
  temporaryVar = 0;
  for (int j = 2; j < 8; j++)
  {
    temporaryVar += statistic[j];
  }
  // do math for count PowerValueNow
  PowerValueNow = round(map(temporaryVar / 6, 0, 1023, 0, 6.2 * 10 * 2));
  // get PowerValueBefore
  PowerValueBefore = EEPROM.read(addrPower);
  // if its diffrent so I neet to send about this to server
  if (abs(PowerValueNow - PowerValueBefore) > 2)
  {
    // write new PowerValueNow in EEPROM
    EEPROM.write(addrPower, PowerValueNow);
    EEPROM.commit();
    SendFlag = 1;
  }
  digitalWrite(transistorPow, LOW);
  // turn on sensor leak
  digitalWrite(transistorLeak, HIGH);
  // make 10 measurements for statistic
  for (int j = 0; j < 10; j++)
  {
    statistic[j] = !digitalRead(leak);
  }
  for (int k = 0; k < 10; k++) // sort
  {
    for (int m = 0; m < 10; m++)
    {
      if (statistic[m] > statistic[k])
      {
        temporaryVar = statistic[m];
        statistic[m] = statistic[k];
        statistic[k] = temporaryVar;
      }
    }
  }
  temporaryVar = 0;
  for (int j = 2; j < 8; j++)
  {
    temporaryVar += statistic[j];
  }
  // do math for count alarm
  alarm = temporaryVar / 6;
  digitalWrite(transistorLeak, LOW);
  // if its "alarm", I neet to send abiut this to server
  if (alarm)
    SendFlag = 1;

  // if SendFlag==1 sen variables to server and go sleep
  if (SendFlag)
  {
    MQTT_Sending();
    MQTT_Sending();
    delay(150);
    WiFi.disconnect();
  }
  ESP.deepSleep(15e6);
}

void loop()
{
}