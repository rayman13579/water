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

const int flow1 = 15;
const int flow2 = 16;
const int flow3 = 17;
const int flow4 = 18;

const int valve1 = 37;
const int valve2 = 36;
const int valve3 = 35;
const int valve4 = 0;

volatile int flowFrequency1 = 0;
volatile int flowFrequency2 = 0;
volatile int flowFrequency3 = 0;
volatile int flowFrequency4 = 0;
int flow_l_min_1 = 0;
int flow_l_min_2 = 0;
int flow_l_min_3 = 0;
int flow_l_min_4 = 0;
unsigned long currentTime;
unsigned long cloopTime;

AsyncWebServer server(80);
ESPDash dashboard(server);


dash::SeparatorCard separator1(dashboard, "Valve States");
dash::FeedbackCard valveCard1(dashboard, "Valve 1");
dash::FeedbackCard valveCard2(dashboard, "Valve 2");
dash::FeedbackCard valveCard3(dashboard, "Valve 3");
dash::FeedbackCard valveCard4(dashboard, "Valve 4");

dash::SeparatorCard separator2(dashboard, "Soil Moisture");
dash::HumidityCard<int, 0> moistCard1(dashboard, "Moisture 1");
dash::HumidityCard<int, 0> moistCard2(dashboard, "Moisture 2");
dash::HumidityCard<int, 0> moistCard3(dashboard, "Moisture 3");
dash::HumidityCard<int, 0> moistCard4(dashboard, "Moisture 4");

dash::SeparatorCard separator3(dashboard, "Water Flow");
dash::HumidityCard<int, 0> flowCard1(dashboard, "Flow 1", "l/min");
dash::HumidityCard<int, 0> flowCard2(dashboard, "Flow 2", "l/min");
dash::HumidityCard<int, 0> flowCard3(dashboard, "Flow 3", "l/min");
dash::HumidityCard<int, 0> flowCard4(dashboard, "Flow 4", "l/min");

void startServer() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wifi", "VeryFastWowy");
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

void updateValveState(dash::FeedbackCard<> &valveCard, int valvePin) {
  if (digitalRead(valvePin)) {
    valveCard.setFeedback("Open", dash::Status::SUCCESS);
  } else {
    valveCard.setFeedback("Closed", dash::Status::NONE);
  }

}

void updateDashboard() {
  updateValveState(valveCard1, valve1);
  updateValveState(valveCard2, valve2);
  updateValveState(valveCard3, valve3);
  updateValveState(valveCard4, valve4);

  moistCard1.setValue(readMoistValue(moist1));
  moistCard2.setValue(readMoistValue(moist2));
  moistCard3.setValue(readMoistValue(moist3));
  moistCard4.setValue(readMoistValue(moist4));

  flowCard1.setValue(flow_l_min_1);
  flowCard2.setValue(flow_l_min_2);
  flowCard3.setValue(flow_l_min_3);
  flowCard4.setValue(flow_l_min_4);

  dashboard.sendUpdates();
}

void flowInterrupt1() {
  flowFrequency1++;
}

void flowInterrupt2() {
  flowFrequency2++;
}

void flowInterrupt3() {
  flowFrequency3++;
}

void flowInterrupt4() {
  flowFrequency4++;
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
  digitalWrite(flow2, HIGH);
  digitalWrite(flow3, HIGH);
  digitalWrite(flow4, HIGH);
  attachInterrupt(flow1, flowInterrupt1, RISING);
  attachInterrupt(flow2, flowInterrupt2, RISING);
  attachInterrupt(flow3, flowInterrupt3, RISING);
  attachInterrupt(flow4, flowInterrupt4, RISING);

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
  delay(500);
  ElegantOTA.loop();
  WebSerial.loop();

  currentTime = millis();
  if (currentTime >= (cloopTime + 1000)) {
    cloopTime = currentTime;
    flow_l_min_1 = flowFrequency1 / 7.5;
    flow_l_min_2 = flowFrequency2 / 7.5;
    flow_l_min_3 = flowFrequency3 / 7.5;
    flow_l_min_4 = flowFrequency4 / 7.5;
    WebSerial.print("Flow frequencies: ");
    WebSerial.print(flowFrequency1);
    WebSerial.print(" | ");
    WebSerial.print(flowFrequency2);
    WebSerial.print(" | ");
    WebSerial.print(flowFrequency3);
    WebSerial.print(" | ");
    WebSerial.println(flowFrequency4);
    flowFrequency1 = 0;
    flowFrequency2 = 0;
    flowFrequency3 = 0;
    flowFrequency4 = 0;
  }

  openValveIfSoilDry(moist1, valve1);
  openValveIfSoilDry(moist2, valve2);
  openValveIfSoilDry(moist3, valve3);
  openValveIfSoilDry(moist4, valve4);

  updateDashboard();
}