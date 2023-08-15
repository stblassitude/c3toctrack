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

char topic[] = "c3toc/train";

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
  int retryCount = 0;

  while (WiFi.status() != WL_CONNECTED || !pubSubClient.connected()) {  
    Serial.print('(');
    Serial.print(WiFi.RSSI());
    Serial.println(')');
  
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print("Reconnecting Wifi: ");
      WiFi.reconnect();

      retryCount = 0;

      while (WiFi.status() != WL_CONNECTED && retryCount < 15) {
        Serial.print(".");
        delay(1000);

        retryCount++;
      }

      if (retryCount < 15)
        Serial.println(WiFi.localIP());
      else
        Serial.println("Timeout connecting");
    }

    // while (!pubSubClient.connected()) {
      Serial.print("Attempting to connect to the MQTT server \"");
      Serial.print(mqttParam.host);
      Serial.print("\", \"");
      Serial.print(mqttParam.user);
      Serial.print("\", \"");
      Serial.print(mqttParam.pass);
      Serial.println("\"...");

      if (!pubSubClient.connect(mqttParam.user, mqttParam.user, mqttParam.pass,
        format_topic(topic, mqttParam.user, "status"), 1, true, "offline")) {
          Serial.print("Connection to MQTT server failed: ");
          Serial.println(pubSubClient.state());
          //WiFi.disconnect();
          delay(5000);
      }
    // }
    if (WiFi.status() == WL_CONNECTED && pubSubClient.connected())
      Serial.println("Connected to the MQTT server");
  }
}


void mqttUpdate() {
  char payload[512];
  char ts[64];
  time_t now = time(nullptr);
  struct tm timeinfo;

  for (;;) {
    if (WiFi.status() != WL_CONNECTED || !pubSubClient.connected()) {
      // wait for main loop to reconnect
      Serial.println("Wait for reconnect!");
      delay(1000);
      continue;
    }
    Serial.print("Updating MQTT ");
    Serial.print("(");
    Serial.print(WiFi.RSSI());
    Serial.print(" dB): ");

    now = time(nullptr);
    gmtime_r(&now, &timeinfo);
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);

    snprintf(payload, sizeof(payload), "alive %s", ts);
    pubSubClient.publish(format_topic(topic, mqttParam.user, "status"), payload);

    snprintf(payload, sizeof(payload), "{\"lat\":%.5f,\"lon\":%.5f,\"sat\":%d,\"speed\":%.1f,\"Vbat\":%.2f,\"Vbus\":%.2f,\"ts\":\"%s\"}",
      gps.location.lat(),
      gps.location.lng(),
      gps.satellites.value(),
      gps.speed.kmph(),
      PMU->getBattVoltage()/1000.0,
      PMU->getVbusVoltage()/1000.0,
      ts);
    pubSubClient.publish(format_topic(topic, mqttParam.user, "pos"), payload);
    Serial.println("sent");

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
  int gpsOk = 0;
  for (;;) {
    while (Serial1.available()) {
      gps.encode(Serial1.read());
      if (gpsOk == 0)
        gpsOk = 1;
    }
    if (gpsOk == 1) {
      Serial.println("GPS data received");
      gpsOk = 2;
    }
    delay(10);
  }
}


BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> heartbeatTask("heartbeat", heartbeat);
BasicCoopTask<CoopTaskStackAllocatorAsMember<32768>> mqttUpdateTask("mqttUpdate", mqttUpdate);
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

  if (PMU->getChipModel() == XPOWERS_AXP192) {
    // lora radio power channel
    PMU->setPowerChannelVoltage(XPOWERS_LDO2, 3300);
    PMU->enablePowerOutput(XPOWERS_LDO2);
    // PMU->disablePowerOutput(XPOWERS_LDO2);

    // oled module power channel,
    // disable it will cause abnormal communication between boot and AXP power supply,
    // do not turn it off
    PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
    // enable oled power
    PMU->enablePowerOutput(XPOWERS_DCDC1);

    // gnss module power channel -  now turned on in setGpsPower
    PMU->setPowerChannelVoltage(XPOWERS_LDO3, 3300);
    // PMU->enablePowerOutput(XPOWERS_LDO3);

    // protected oled power source
    PMU->setProtectedChannel(XPOWERS_DCDC1);
    // protected esp32 power source
    PMU->setProtectedChannel(XPOWERS_DCDC3);

    // disable not use channel
    PMU->disablePowerOutput(XPOWERS_DCDC2);

    // disable all axp chip interrupt
    PMU->disableIRQ(XPOWERS_AXP192_ALL_IRQ);

    // Set constant current charging current
    PMU->setChargerConstantCurr(XPOWERS_AXP192_CHG_CUR_450MA);

    // Set up the charging voltage
    PMU->setChargeTargetVoltage(XPOWERS_AXP192_CHG_VOL_4V2);
  } else if (PMU->getChipModel() == XPOWERS_AXP2101) {
    // Unuse power channel
    PMU->disablePowerOutput(XPOWERS_DCDC2);
    PMU->disablePowerOutput(XPOWERS_DCDC3);
    PMU->disablePowerOutput(XPOWERS_DCDC4);
    PMU->disablePowerOutput(XPOWERS_DCDC5);
    PMU->disablePowerOutput(XPOWERS_ALDO1);
    PMU->disablePowerOutput(XPOWERS_ALDO4);
    PMU->disablePowerOutput(XPOWERS_BLDO1);
    PMU->disablePowerOutput(XPOWERS_BLDO2);
    PMU->disablePowerOutput(XPOWERS_DLDO1);
    PMU->disablePowerOutput(XPOWERS_DLDO2);

    // GNSS RTC PowerVDD 3300mV
    PMU->setPowerChannelVoltage(XPOWERS_VBACKUP, 3300);
    PMU->enablePowerOutput(XPOWERS_VBACKUP);

    // ESP32 VDD 3300mV
    //  ! No need to set, automatically open , Don't close it
    //  PMU->setPowerChannelVoltage(XPOWERS_DCDC1, 3300);
    //  PMU->setProtectedChannel(XPOWERS_DCDC1);

    // LoRa VDD 3300mV
    PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO2);

    // GNSS VDD 3300mV
    PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);
    PMU->enablePowerOutput(XPOWERS_ALDO3);
  }

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
  pubSubClient.loop();
  mqttConnect();
  delay(10);
}
