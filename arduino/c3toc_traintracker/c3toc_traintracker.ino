#include <ArduinoMqttClient.h>
#include <axp20x.h>
#include <CoopTask.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <WiFiManager.h>

const int LED = 4;
const int BUTTON = 38;

TinyGPSPlus gps;
AXP20X_Class axp;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

struct mqttParam {
  char valid;
  char host[40];
  char user[20];
  char pass[20];
} mqttParam;

int wifiParamsUpdated = 0;

String topic = "c3toc/demo";


void loadMqttParam() {
  EEPROM.begin(sizeof(mqttParam));
  char *p = (char *)&mqttParam;
  for (int i = 0; i < sizeof(mqttParam); i++) {
    *p++ = EEPROM.readChar(i);
  }
  EEPROM.end();
}

void saveMqttParam() {
  EEPROM.begin(sizeof(mqttParam));
  char *p = (char *)&mqttParam;
  for (int i = 0; i < sizeof(mqttParam); i++) {
    EEPROM.writeChar(i, *p++);
  }
  EEPROM.commit();
  EEPROM.end();
}

void gpsDebug() {
  Serial.print("chars: ");
  Serial.println(gps.charsProcessed());

  Serial.print("Latitude  : ");
  Serial.println(gps.location.lat(), 5);
  Serial.print("Longitude : ");
  Serial.println(gps.location.lng(), 4);
  Serial.print("Satellites: ");
  Serial.println(gps.satellites.value());
  Serial.print("Altitude  : ");
  Serial.print(gps.altitude.feet() / 3.2808);
  Serial.println("M");
  Serial.print("Time      : ");
  Serial.print(gps.time.hour());
  Serial.print(":");
  Serial.print(gps.time.minute());
  Serial.print(":");
  Serial.println(gps.time.second());
  Serial.println("**********************");
}

void heartbeat() {
  pinMode(LED, OUTPUT);

  for (;;) {
    digitalWrite(LED, LOW);
    delay(200);
    digitalWrite(LED, HIGH);
    delay(800);

    //gpsDebug();
  }
}


void mqttLastWill() {
  String willPayload = "offline";
  mqttClient.beginWill(topic + "/" + mqttParam.user + "/status", willPayload.length(), true, 1);
  mqttClient.print(willPayload);
  mqttClient.endWill();
  Serial.println("Updated last will");
}

void mqttAlive() {
  mqttClient.setId(mqttParam.user);
  mqttClient.setUsernamePassword(mqttParam.user, mqttParam.pass);
  mqttLastWill();

  for (;;) {
    while (!mqttClient.connected()) {
      Serial.print("Attempting to connect to the MQTT broker: \"");
      Serial.print(mqttParam.host);
      Serial.print("\"\r\nuser: \"");
      Serial.print(mqttParam.user);
      Serial.print("\"\r\npass: \"");
      Serial.print(mqttParam.pass);
      Serial.print("\"\r\n");

      if (mqttClient.connect(mqttParam.host, 1883)) {
        Serial.println("You're connected to the MQTT broker!");
      } else {
        Serial.print("MQTT connection failed! Error code = ");
        Serial.println(mqttClient.connectError());
        delay(10000);
      }
    }

    mqttClient.beginMessage(topic + "/" + mqttParam.user + "/status");
    mqttClient.print("alive");
    mqttClient.endMessage();
    Serial.println("Sent alive");

    mqttClient.beginMessage(topic + "/" + mqttParam.user + "/pos");
    mqttClient.print("{\"lat\":");
    mqttClient.print(gps.location.lat(),5);
    mqttClient.print(",\"lon\":");
    mqttClient.print(gps.location.lng(),5);
    mqttClient.print(",\"sat\":");
    mqttClient.print(gps.satellites.value());
    mqttClient.print(",\"speed\":");
    mqttClient.print(gps.speed.kmph());
    mqttClient.print("}");
    mqttClient.endMessage();
    Serial.println("Sent alive");

    delay(10000);
  }
}

void reconfigure() {
  pinMode(BUTTON, INPUT);

  for (;;) {
    if (!digitalRead(BUTTON)) {
      Serial.println("Starting reconfiguration");
      WiFi.disconnect(true, true);
      delay(1000);
      ESP.restart();
    }
    delay(10);
  }
}

BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> heartbeatTask("heartbeat", heartbeat);
BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> mqttAliveTask("mqttAlive", mqttAlive);
BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> reconfigureTask("reconfigure", reconfigure);


void wifiParamsUpdatedCallback() {
  wifiParamsUpdated = 1;
}

void setupWifi() { 
  int p = 0;

  pinMode(BUTTON, INPUT);

  loadMqttParam();

  WiFiManagerParameter custom_mqtt_host("host", "MQTT Host", mqttParam.host, sizeof(mqttParam.host));
  WiFiManagerParameter custom_mqtt_user("user", "MQTT Username", mqttParam.user, sizeof(mqttParam.user));
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Password", mqttParam.pass, sizeof(mqttParam.pass));

  WiFiManager wm;
  wm.setSaveConfigCallback(wifiParamsUpdatedCallback);
  wm.addParameter(&custom_mqtt_host);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);

  if (mqttParam.valid != 23 || mqttParam.host[0] == 0xff || mqttParam.user[0] == 0xff || mqttParam.pass[0] == 0xff) {
    Serial.println("Invalid MQTT config, starting portal");
    mqttParam.host[0] = 0;
    mqttParam.user[0] = 0;
    mqttParam.pass[0] = 0;
    wm.startConfigPortal();
  } else {
    if (!digitalRead(BUTTON)) {
      wm.startConfigPortal();
    } else {
      if (wm.autoConnect()) {
        Serial.println("Connected to Wifi.");
      } else {
        Serial.println("Unable to connect Wifi, restarting...\n\n");
        ESP.restart();
      }
    }
  }

  if (wifiParamsUpdated) {
    mqttParam.valid = 23;
    strlcpy(mqttParam.host, custom_mqtt_host.getValue(), sizeof(mqttParam.host));
    strlcpy(mqttParam.user, custom_mqtt_user.getValue(), sizeof(mqttParam.user));
    strlcpy(mqttParam.pass, custom_mqtt_pass.getValue(), sizeof(mqttParam.pass));
    saveMqttParam();

    Serial.print("Saving MQTT parameters\nhost: \"");
    Serial.print(mqttParam.host);
    Serial.print("\"\r\nuser: \"");
    Serial.print(mqttParam.user);
    Serial.print("\"\r\npass: \"");
    Serial.print(mqttParam.pass);
    Serial.print("\"\r\n");
  }
}

void setup() {
  Serial.begin(230400);
  Serial.println("\n\rc3toc train tracker booting...\n\r");

  // GPS chip
  //Serial1.begin(9600, SERIAL_8N1, 12, 15);  //17-TX 18-RX
  Serial1.begin(9600, SERIAL_8N1, 34, 12, false, 1000);

  // power
  Wire1.begin(21, 22);
  axp.begin(Wire, AXP192_SLAVE_ADDRESS);
  axp.setChgLEDMode (AXP20X_LED_OFF); 
  axp.setPowerOutPut(AXP192_LDO2, AXP202_OFF); // LORA
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON); // GPS
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);

  setupWifi();

  heartbeatTask.scheduleTask();
  reconfigureTask.scheduleTask();
  mqttAliveTask.scheduleTask();
}

void loop() {
  runCoopTasks();
  mqttClient.poll();
  while (Serial1.available())
    gps.encode(Serial1.read());
}
