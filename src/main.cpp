#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPDashboardPlus.h>
#include "dashboard_html.h"
#include <Ticker.h>

// ADC2 can not be used when WiFi enabled

const int MAX_DRY_VALUE = 3333;
const int MIN_DRY_VALUE = 1140;

const int moist1 = 4;
const int moist2 = 5;
const int moist3 = 6;
const int moist4 = 7;

const int flow1 = 11;
const int flow2 = 12;
const int flow3 = 14;
const int flow4 = 13;

const int valve1 = 16;
const int valve2 = 18;
const int valve3 = 15;
const int valve4 = 17;

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

Ticker flowCalculationTicker;
Ticker moistureChartTicker;

AsyncWebServer server(80);
ESPDashboardPlus dashboard("Water");

void startServer()
{
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin("Wifi", "VeryFastWowy");
  if (WiFi.waitForConnectResult(5000) != WL_CONNECTED)
  {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello, world"); });

  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404, "text/plain", "Not found"); });
  dashboard.begin(&server, DASHBOARD_HTML_DATA, DASHBOARD_HTML_SIZE, true, true);

  dashboard.addStatusCard("valve1", "Valve 1", StatusIcon::POWER);
  dashboard.addStatusCard("valve2", "Valve 2", StatusIcon::POWER);
  dashboard.addStatusCard("valve3", "Valve 3", StatusIcon::POWER);
  dashboard.addStatusCard("valve4", "Valve 4", StatusIcon::POWER);

  ChartCard *moistChart1 = dashboard.addChartCard("moistChart1", "Moisture History", ChartType::LINE, 100);
  moistChart1->setSize(1, 1);
  ChartCard *moistChart2 = dashboard.addChartCard("moistChart2", "Moisture History", ChartType::LINE, 100);
  moistChart2->setSize(1, 1);
  ChartCard *moistChart3 = dashboard.addChartCard("moistChart3", "Moisture History", ChartType::LINE, 100);
  moistChart3->setSize(1, 1);
  ChartCard *moistChart4 = dashboard.addChartCard("moistChart4", "Moisture History", ChartType::LINE, 100);
  moistChart4->setSize(1, 1);
  //  moistChart->addSeries("Moisture 1", "#3700ffff");
  //  moistChart->addSeries("Moisture 2", "#00FF00");
  //  moistChart->addSeries("Moisture 3", "#FFFF00");
  //  moistChart->addSeries("Moisture 4", "#FF0000");

  dashboard.addStatCard("moist1", "Moisture 1", "%");
  dashboard.addStatCard("moist2", "Moisture 2", "%");
  dashboard.addStatCard("moist3", "Moisture 3", "%");
  dashboard.addStatCard("moist4", "Moisture 4", "%");

  dashboard.addStatCard("flow1", "Flow 1", "l/min");
  dashboard.addStatCard("flow2", "Flow 2", "l/min");
  dashboard.addStatCard("flow3", "Flow 3", "l/min");
  dashboard.addStatCard("flow4", "Flow 4", "l/min");

  dashboard.addGroup("valve", "Valve States", {"valve1", "valve2", "valve3", "valve4"});
  dashboard.addGroup("moistureChart", "Soil Moisture", {"moistChart1", "moistChart2", "moistChart3", "moistChart4"});
  dashboard.addGroup("moisture", "Soil Moisture", {"moist1", "moist2", "moist3", "moist4"});
  dashboard.addGroup("flow", "Water Flow", {"flow1", "flow2", "flow3", "flow4"});

  server.begin();
  Serial.println("HTTP server started");
}

int readMoistValue(int input)
{
  int value = analogRead(input);
  return constrain(map(value, MIN_DRY_VALUE, MAX_DRY_VALUE, 100, 0), 0, 100);
}

void openValveIfSoilDry(int sensor, int valve)
{
  if (readMoistValue(sensor) < 50)
  {
    digitalWrite(valve, HIGH);
  }
  else
  {
    digitalWrite(valve, LOW);
  }
}

void updateValveState(int valvePin, String valveCard)
{
  if (digitalRead(valvePin))
  {
    dashboard.updateStatusCard(valveCard, StatusIcon::POWER, CardVariant::INFO, "Open", "");
  }
  else
  {
    dashboard.updateStatusCard(valveCard, StatusIcon::POWER, CardVariant::SECONDARY, "Closed", "");
  }
}

void updateDashboard()
{
  updateValveState(valve1, "valve1");
  updateValveState(valve2, "valve2");
  updateValveState(valve3, "valve3");
  updateValveState(valve4, "valve4");

  dashboard.updateStatCard("moist1", String(readMoistValue(moist1)));
  dashboard.updateStatCard("moist2", String(readMoistValue(moist2)));
  dashboard.updateStatCard("moist3", String(readMoistValue(moist3)));
  dashboard.updateStatCard("moist4", String(readMoistValue(moist4)));

  dashboard.updateStatCard("flow1", String(flow_l_min_1));
  dashboard.updateStatCard("flow2", String(flow_l_min_2));
  dashboard.updateStatCard("flow3", String(flow_l_min_3));
  dashboard.updateStatCard("flow4", String(flow_l_min_4));
}

void IRAM_ATTR flowInterrupt1()
{
  flowFrequency1 = flowFrequency1 + 1;
}

void IRAM_ATTR flowInterrupt2()
{
  flowFrequency2 = flowFrequency2 + 1;
}

void IRAM_ATTR flowInterrupt3()
{
  flowFrequency3 = flowFrequency3 + 1;
}

void IRAM_ATTR flowInterrupt4()
{
  flowFrequency4 = flowFrequency4 + 1;
}

void calculateFlow()
{
  flow_l_min_1 = flowFrequency1 / 7.5;
  flow_l_min_2 = flowFrequency2 / 7.5;
  flow_l_min_3 = flowFrequency3 / 7.5;
  flow_l_min_4 = flowFrequency4 / 7.5;
  dashboard.logInfo("Flow frequencies: " + String(flow_l_min_1) + " | " + String(flow_l_min_2) + " | " + String(flow_l_min_3) + " | " + String(flow_l_min_4));
  flowFrequency1 = 0;
  flowFrequency2 = 0;
  flowFrequency3 = 0;
  flowFrequency4 = 0;

  openValveIfSoilDry(moist1, valve1);
  openValveIfSoilDry(moist2, valve2);
  openValveIfSoilDry(moist3, valve3);
  openValveIfSoilDry(moist4, valve4);

  updateDashboard();
}

void updateMoistureCharts()
{
  dashboard.updateChartCard("moistChart1", readMoistValue(moist1));
  dashboard.updateChartCard("moistChart2", readMoistValue(moist2));
  dashboard.updateChartCard("moistChart3", readMoistValue(moist3));
  dashboard.updateChartCard("moistChart4", readMoistValue(moist4));
}

void setup()
{
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

  flowCalculationTicker.attach(10, calculateFlow);
  moistureChartTicker.attach(3600, updateMoistureCharts);

  sei();
}

void loop()
{
  dashboard.loop();
  delay(10);
}