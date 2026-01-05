#include <Arduino.h>
#include <WiFi.h>
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"

AsyncWebServer server(80);

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

void startServer() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wokwi-GUEST", "", 6);
  if (WiFi.waitForConnectResult(5000) != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    exit(1);
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
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
  return analogRead(muxOutput);
}

void openValveIfSoilDry(int sensor, int valve) {
  if (readFromInput(sensor) < 2000) {
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

void loop() {
  delay(10); // this speeds up the simulation

 // printSensorStats();

  openValveIfSoilDry(moist1, valve1);
  openValveIfSoilDry(moist2, valve2);
  openValveIfSoilDry(moist3, valve3);
  openValveIfSoilDry(moist4, valve4);
}