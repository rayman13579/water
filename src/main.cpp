#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDash.h>


const int MAX_SENSOR_VALUE = 4095;

const int muxEnable = 1;
const int muxOutput = 2;
const int mux0 = 7;
const int mux1 = 6;
const int mux2 = 5;
const int mux3 = 4;

const int moist1 = 0;
const int moist2 = 1;
const int moist3 = 2;
const int moist4 = 3;

const int flow1 = 15;
const int flow2 = 14;
const int flow3 = 13;
const int flow4 = 12;

const int valve1 = 38;
const int valve2 = 37;
const int valve3 = 36;
const int valve4 = 35;


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
  WiFi.begin("Wokwi-GUEST", "", 6);
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

int readFromInput(int input) {
  digitalWrite(mux0, bitRead(input, 0));
  digitalWrite(mux1, bitRead(input, 1));
  digitalWrite(mux2, bitRead(input, 2));
  digitalWrite(mux3, bitRead(input, 3));
  delay(5);
  int value = analogRead(muxOutput);
  return (value * 100) / MAX_SENSOR_VALUE;
}

void openValveIfSoilDry(int sensor, int valve) {
  if (readFromInput(sensor) < 50) {
    digitalWrite(valve, HIGH);
  } else {
    digitalWrite(valve, LOW);
  }
}

void printSensorStats() {
  Serial.print("Moist: ");
  Serial.print(readFromInput(moist1));
  Serial.print(" | ");
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
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, ESP32-S3!");
  pinMode(muxOutput, INPUT);
  pinMode(muxEnable, OUTPUT);
  pinMode(mux0, OUTPUT);
  pinMode(mux1, OUTPUT);
  pinMode(mux2, OUTPUT);
  pinMode(mux3, OUTPUT);

  pinMode(valve1, OUTPUT);
  pinMode(valve2, OUTPUT);
  pinMode(valve3, OUTPUT);
  pinMode(valve4, OUTPUT);

  digitalWrite(muxEnable, LOW);
  digitalWrite(mux0, LOW);
  digitalWrite(mux1, LOW);
  digitalWrite(mux2, LOW);
  digitalWrite(mux3, LOW);

  digitalWrite(valve1, LOW);
  digitalWrite(valve2, LOW);
  digitalWrite(valve3, LOW);
  digitalWrite(valve4, LOW);

  startServer();
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

  moistCard1.setValue(readFromInput(moist1));
  moistCard2.setValue(readFromInput(moist2));
  moistCard3.setValue(readFromInput(moist3));
  moistCard4.setValue(readFromInput(moist4));

  flowCard1.setValue(readFromInput(flow1));
  flowCard2.setValue(readFromInput(flow2));
  flowCard3.setValue(readFromInput(flow3));
  flowCard4.setValue(readFromInput(flow4));

  dashboard.sendUpdates();
}

void loop() {
  delay(50);

 // printSensorStats();

  openValveIfSoilDry(moist1, valve1);
  openValveIfSoilDry(moist2, valve2);
  openValveIfSoilDry(moist3, valve3);
  openValveIfSoilDry(moist4, valve4);

  updateDashboard();
}