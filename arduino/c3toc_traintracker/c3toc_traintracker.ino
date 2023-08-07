#include <ArduinoMqttClient.h>
#include <CoopTask.h>
#include <EEPROM.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include <XPowersLib.h>

const int LED = 4;
const int BUTTON = 38;

TinyGPSPlus gps;
//XPowersPMU PMU;
XPowersLibInterface *PMU = NULL;

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

    mqttClient.beginMessage(topic + "/" + mqttParam.user + "/pos");
    mqttClient.print("{\"lat\":");
    mqttClient.print(gps.location.lat(),5);
    mqttClient.print(",\"lon\":");
    mqttClient.print(gps.location.lng(),5);
    mqttClient.print(",\"sat\":");
    mqttClient.print(gps.satellites.value());
    mqttClient.print(",\"speed\":");
    mqttClient.print(gps.speed.kmph());
    mqttClient.print(",\"Vbat\":");
    mqttClient.print(PMU->getBattVoltage());
    mqttClient.print(",\"Vbus\":");
    mqttClient.print(PMU->getVbusVoltage());
    mqttClient.print(",\"Vsys\":");
    mqttClient.print(PMU->getSystemVoltage());
    mqttClient.print("}");
    mqttClient.endMessage();
    Serial.println("Sent MQTT messages");

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


void power() {
  while(1) {
    uint32_t status = PMU->getIrqStatus();
    // Serial.print("STATUS => HEX:");
    // Serial.print(status, HEX);
    // Serial.print(",\"Vbat\":");
    // Serial.println(status, BIN);
    PMU->clearIrqStatus();
    delay(1000);
  }
}

BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> heartbeatTask("heartbeat", heartbeat);
BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> mqttAliveTask("mqttAlive", mqttAlive);
BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> reconfigureTask("reconfigure", reconfigure);
BasicCoopTask<CoopTaskStackAllocatorAsMember<2000>> powerTask("power", power);


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
  mqttAliveTask.scheduleTask();
  powerTask.scheduleTask();
}

void loop() {
  runCoopTasks();
  mqttClient.poll();
  while (Serial1.available())
    gps.encode(Serial1.read());
}
