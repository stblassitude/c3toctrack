#include <CoopTask.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>

#include <XPowersLib.h>

const int LED = 4;
const int BUTTON = 38;

TinyGPSPlus gps;
//XPowersPMU PMU;
XPowersLibInterface *PMU = NULL;

// WiFiClient wifiClient;
WiFiClientSecure wifiClient;
// MqttClient mqttClient(wifiClient);
PubSubClient pubSubClient(wifiClient);

struct mqttParam {
  char valid;
  char host[40];
  char user[20];
  char pass[20];
} mqttParam;

int wifiParamsUpdated = 0;

char topic[] = "c3toc/demo";

const char* root_ca= \
     "-----BEGIN CERTIFICATE-----\n" \
     "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\n" \
     "-----END CERTIFICATE-----\n";


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


char *format_topic(const char *t, const char *u, const char *i) {
  static char s[64];
  snprintf(s, sizeof(s), "%s/%s/%s", t, u, i);
  return s;
}


void mqttConnect() {
  while (!pubSubClient.connected()) {
    Serial.println("Attempting to connect to the MQTT server");
    while (!pubSubClient.connect(mqttParam.user, mqttParam.user, mqttParam.pass,
      format_topic(topic, "mqttParam.user", "status"), 1, true, "offline")) {
        Serial.print("Connection to MQTT server failed: ");
        Serial.println(pubSubClient.state());
        delay(5000);
    }
    Serial.println("Connected to the MQTT server");
  }
}


void mqttUpdate() {
  char payload[512];
  char ts[64];
  time_t now = time(nullptr);
  struct tm timeinfo;

  for (;;) {
    if (!pubSubClient.connected()) {
      delay(1000);
      continue;
    }
    now = time(nullptr);
    gmtime_r(&now, &timeinfo);
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

    snprintf(payload, sizeof(payload), "alive %s", ts);
    pubSubClient.publish(format_topic(topic, "mqttParam.user", "status"), payload);

    snprintf(payload, sizeof(payload), "{\"lat\":%.5f,\"lon\":%.5f,\"sat\":%d,\"speed\":%.1f,\"Vbat\":%.2f,\"Vbus\":%.2f,\"ts\":\"%s\"}",
      gps.location.lat(),
      gps.location.lng(),
      gps.satellites.value(),
      gps.speed.kmph(),
      PMU->getBattVoltage()/1000.0,
      PMU->getVbusVoltage()/1000.0,
      ts);
    pubSubClient.publish(format_topic(topic, "mqttParam.user", "pos"), payload);

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


void gpsAlive() {
  for (;;) {
    while (Serial1.available())
      gps.encode(Serial1.read());
    delay(10);
  }
}


BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> heartbeatTask("heartbeat", heartbeat);
BasicCoopTask<CoopTaskStackAllocatorAsMember<16384>> mqttUpdateTask("mqttUpdate", mqttUpdate);
BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> reconfigureTask("reconfigure", reconfigure);
BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> gpsTask("gps", gpsAlive);




void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}


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

  setClock();
}


void setupPower() {
 if (!PMU) {
    PMU = new XPowersAXP2101(Wire);
    if (PMU->init()) {
      Serial.println("Detected AXP2101");
    } else {
      PMU = NULL;
    }
  }
  if (!PMU) {
    PMU = new XPowersAXP192(Wire);
    if (PMU->init()) {
      Serial.println("Detected AXP192");
    } else {
      PMU = NULL;
    }
  }
  if (!PMU) {
      Serial.println("No PMIC detected, stopping");
      while (1) delay(1000);
  }
  Serial.print("AXP Chip ID: ");
  Serial.println(PMU->getChipID());

  // PMU->setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
  // PMU->setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);
  // PMU->setDC1Voltage(3300);
  // PMU->disableDC2();
  // PMU->enableDC3();
  // PMU->enableDC4();
  // PMU->enableDC5();
  // PMU->enableALDO1();
  // PMU->enableALDO2();
  // PMU->enableALDO3();
  // PMU->enableALDO4();
  // PMU->enableBLDO1();
  // PMU->enableBLDO2();
  // PMU->enableCPUSLDO();
  // PMU->enableDLDO1();
  // PMU->enableDLDO2();

  // PMU->disablePowerOutput(XPOWERS_DCDC2);
  // PMU->disablePowerOutput(XPOWERS_DCDC3);
  // PMU->disablePowerOutput(XPOWERS_DCDC4);
  // PMU->disablePowerOutput(XPOWERS_DCDC5);
  // PMU->disablePowerOutput(XPOWERS_ALDO1);
  // PMU->disablePowerOutput(XPOWERS_ALDO4);
  // PMU->disablePowerOutput(XPOWERS_BLDO1);
  // PMU->disablePowerOutput(XPOWERS_BLDO2);
  // PMU->disablePowerOutput(XPOWERS_DLDO1);
  // PMU->disablePowerOutput(XPOWERS_DLDO2);
  // PMU->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
  // PMU->enablePowerOutput(XPOWERS_VBACKUP);

  // LoRa VDD 3300mV
  // PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
  // PMU->enablePowerOutput(XPOWERS_ALDO2);

  // GNSS VDD 3300mV
  // PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
  // PMU->enablePowerOutput(XPOWERS_ALDO3);

  PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

  // PMU->enableBattDetection();
  // PMU->enableVbusVoltageMeasure();
  // PMU->enableBattVoltageMeasure();
  // PMU->enableSystemVoltageMeasure();

  PMU->enableVbusVoltageMeasure();
  PMU->enableBattVoltageMeasure();
  PMU->enableSystemVoltageMeasure();

  PMU->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);
  // PMU->setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_200MA);
  // PMU->setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
  // PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_1000MA);
  // PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
  // PMU->setLowBatWarnThreshold(10);
  // PMU->setLowBatShutdownThreshold(5);
  // PMU->enableCellbatteryCharge();

  // Disable all interrupts
  PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  // PMU->fuelGaugeControl(true, true);
}


void setup() {
  Serial.begin(230400);
  Serial.println("\n\rc3toc train tracker booting...\n\r");

  // GPS chip
  Serial1.begin(9600, SERIAL_8N1, 34, 12, false, 1000);

  Wire.begin(21, 22);

  setupPower();

  setupWifi();

  heartbeatTask.scheduleTask();
  reconfigureTask.scheduleTask();
  mqttUpdateTask.scheduleTask();
  gpsTask.scheduleTask();

  wifiClient.setInsecure();
  pubSubClient.setServer(mqttParam.host, 8883);
}

void loop() {
  runCoopTasks();
  mqttConnect();
  pubSubClient.loop();
  delay(10);
}
