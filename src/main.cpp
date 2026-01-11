#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>
#include <ElegantOTA.h>
#include <WebSerial.h>


//ADC2 can not be used when WiFi enabled

const int MAX_DRY_VALUE = 3333;
const int MIN_DRY_VALUE = 1140;

const int moist1 = 4;
const int moist2 = 5;
const int moist3 = 6;
const int moist4 = 7;

const int flow1 = 10;
const int flow2 = 9;
const int flow3 = 3;
const int flow4 = 8;

const int valve1 = 38;
const int valve2 = 37;
const int valve3 = 36;
const int valve4 = 35;

volatile int flowFrequency1 = 0;
int flow_l_min_1 = 0;
unsigned long currentTime;
unsigned long cloopTime;

AsyncWebServer server(80);
ESPDash dashboard(server);


dash::SeparatorCard separator3(dashboard, "Valve States");
dash::FeedbackCard valveCard1(dashboard, "Valve 1");
dash::FeedbackCard valveCard2(dashboard, "Valve 2");
dash::FeedbackCard valveCard3(dashboard, "Valve 3");
dash::FeedbackCard valveCard4(dashboard, "Valve 4");

dash::SeparatorCard separator(dashboard, "Soil Moisture");
dash::HumidityCard<int, 0> moistCard1(dashboard, "Moisture 1");
dash::HumidityCard<int, 0> moistCard2(dashboard, "Moisture 2");
dash::HumidityCard<int, 0> moistCard3(dashboard, "Moisture 3");
dash::HumidityCard<int, 0> moistCard4(dashboard, "Moisture 4");

dash::SeparatorCard separator2(dashboard, "Water Flow");
dash::HumidityCard<int, 0> flowCard1(dashboard, "Flow 1", "l/min");
dash::HumidityCard<int, 0> flowCard2(dashboard, "Flow 2", "l/min");
dash::HumidityCard<int, 0> flowCard3(dashboard, "Flow 3", "l/min");
dash::HumidityCard<int, 0> flowCard4(dashboard, "Flow 4", "l/min");

void startServer() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wifi", "pw");
  if (WiFi.waitForConnectResult(5000) != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello, world");
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
  });

  server.begin();
  Serial.println("HTTP server started");
}

int readMoistValue(int input) {
  int value = analogRead(input);
  return constrain(map(value, MIN_DRY_VALUE, MAX_DRY_VALUE, 100, 0), 0, 100);
}

void openValveIfSoilDry(int sensor, int valve) {
  if (readMoistValue(sensor) < 50) {
    digitalWrite(valve, HIGH);
  } else {
    digitalWrite(valve, LOW);
  }
}

void printSensorStats() {
 /* Serial.print(" | ");
  Serial.print(readFromInput(moist1));
  Serial.print(" | ");
  Serial.print(readFromInput(moist2));
  Serial.print(" | ");
  Serial.print(readFromInput(moist3));
  Serial.println();
  Serial.print("Flow: ");
  Serial.print(readFromInput(flow1));
  Serial.print(" | ");
  Serial.print(readFromInput(flow2));
  Serial.print(" | ");
  Serial.print(readFromInput(flow3));
  Serial.print(" | ");
  Serial.print(readFromInput(flow4));
  Serial.println();
*/
}

void updateValveState(dash::FeedbackCard<> &valveCard, int valvePin) {
  if (digitalRead(valvePin)) {
    valveCard.setFeedback("Open", dash::Status::SUCCESS);
  } else {
    valveCard.setFeedback("Closed", dash::Status::NONE);
  }

}

void updateDashboard() {
  updateValveState(valveCard1, valve1);
//  updateValveState(valveCard2, valve2);
//  updateValveState(valveCard3, valve3);
//  updateValveState(valveCard4, valve4);

  moistCard1.setValue(readMoistValue(moist1));
//  moistCard2.setValue(readMoistValue(moist2));
//  moistCard3.setValue(readMoistValue(moist3));
//  moistCard4.setValue(readMoistValue(moist4));

  flowCard1.setValue(flow_l_min_1);
//  flowCard2.setValue(readMoistValue(flow2));
//  flowCard3.setValue(readMoistValue(flow3));
//  flowCard4.setValue(readMoistValue(flow4));

  dashboard.sendUpdates();
}

void flowInterrupt1() {
  flowFrequency1++;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Hello, ESP32-S3!");
  pinMode(moist1, INPUT);
  pinMode(moist2, INPUT);
  pinMode(moist3, INPUT); 
  pinMode(moist4, INPUT);

  pinMode(flow1, INPUT);
  pinMode(flow2, INPUT);
  pinMode(flow3, INPUT);
  pinMode(flow4, INPUT);

  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(valve3, OUTPUT);
  pinMode(valve4, OUTPUT);

  digitalWrite(flow1, HIGH);
  attachInterrupt(flow1, flowInterrupt1, RISING);

  digitalWrite(valve1, LOW);
  digitalWrite(valve2, LOW);
  digitalWrite(valve3, LOW);
  digitalWrite(valve4, LOW);

  startServer();
  ElegantOTA.begin(&server);
  WebSerial.begin(&server);
  
  sei();
  currentTime = millis();
  cloopTime = millis();
}

void loop() {
  delay(250);
  ElegantOTA.loop();
  WebSerial.loop();

  currentTime = millis();
  if (currentTime >= (cloopTime + 1000)) {
    cloopTime = currentTime;
    flow_l_min_1 = flowFrequency1 / 7.5;
    WebSerial.print("Flow 1 frequency: ");
    WebSerial.print(flowFrequency1);
    WebSerial.print(" | l/min: ");
    WebSerial.println(flow_l_min_1);
    flowFrequency1 = 0;
  }

  printSensorStats();

  openValveIfSoilDry(moist1, valve1);
//  openValveIfSoilDry(moist2, valve2);
//  openValveIfSoilDry(moist3, valve3);
//  openValveIfSoilDry(moist4, valve4);

  updateDashboard();
}