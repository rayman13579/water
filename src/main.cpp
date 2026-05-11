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

std::vector<int> valves = {valve1, valve2, valve3, valve4};

volatile int flowFrequency1 = 0;
volatile int flowFrequency2 = 0;
volatile int flowFrequency3 = 0;
volatile int flowFrequency4 = 0;
int flow_l_min_1 = 0;
int flow_l_min_2 = 0;
int flow_l_min_3 = 0;
int flow_l_min_4 = 0;

Ticker flowCalculationTicker;
Ticker moistureChartTicker;

bool doDashboardUpdate = false;
bool doMoistureChartUpdate = false;

bool doWatering = false;
int selectedValve = 0;
int wateringDuration = 1;
unsigned long wateringEndTime = 0;

AsyncWebServer server(80);
ESPDashboardPlus dashboard("Water");

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

void setupPins()
{
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
}

void setupServer()
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
  { 
    request->send(200, "text/plain", "Hello, world"); 
  });

  server.onNotFound([](AsyncWebServerRequest *request)
  { 
    request->send(404, "text/plain", "Not found");
  });
  server.begin();
  Serial.println("HTTP server started");
}

void setupDashboard()
{
  dashboard.begin(&server, DASHBOARD_HTML_DATA, DASHBOARD_HTML_SIZE, true, true);

  DropdownCard *valveDropdown = dashboard.addDropdownCard("valveDropdown", "Valve", "Select valve");
  valveDropdown->addOption("0", "Valve 1");
  valveDropdown->addOption("1", "Valve 2");
  valveDropdown->addOption("2", "Valve 3");
  valveDropdown->addOption("3", "Valve 4");
  valveDropdown->onChange = [](const String &value)
  {
    selectedValve = value.toInt();
    dashboard.updateButtonCard("doWater", "open valve " + String(selectedValve + 1) + " for " + String(wateringDuration) + " seconds");
  };
  valveDropdown->setValue("0");
  SliderCard *durationSlider = dashboard.addSliderCard("durationSlider", "Watering Duration", 1, 10, 1, "sec");
  durationSlider->onChange = [](int value)
  { 
    wateringDuration = value;
    dashboard.updateButtonCard("doWater", "open valve " + String(selectedValve + 1) + " for " + String(wateringDuration) + " seconds");
  };
  dashboard.addButtonCard("doWater", "Water", "open valve 1 for 1 seconds", true, []
  {
    dashboard.updateButtonCard("doWater", false);
    wateringEndTime = millis() + wateringDuration * 1000;
    doWatering = true;
    digitalWrite(valves[selectedValve], HIGH);
  });
  ActionButton *restartButton = dashboard.addActionButton("restart", "Restart", "Restart", "Restart?", "", []()
  { 
    ESP.restart();
  });
  restartButton->setVariant(CardVariant::DANGER);

  StatusToggleCard *valveToggle1 = dashboard.addStatusToggleCard("valve1", "Valve 1", "Open", "Closed", StatusIcon::RAIN, StatusIcon::CLOUD, false);
  valveToggle1->setVariant(CardVariant::INFO);
  valveToggle1->onChange = [](bool value)
  {
    digitalWrite(valve1, value ? HIGH : LOW);
  };
  StatusToggleCard *valveToggle2 = dashboard.addStatusToggleCard("valve2", "Valve 2", "Open", "Closed", StatusIcon::RAIN, StatusIcon::CLOUD, false);
  valveToggle2->setVariant(CardVariant::INFO);
  valveToggle2->onChange = [](bool value)
  {
    digitalWrite(valve2, value ? HIGH : LOW);
  };
  StatusToggleCard *valveToggle3 = dashboard.addStatusToggleCard("valve3", "Valve 3", "Open", "Closed", StatusIcon::RAIN, StatusIcon::CLOUD, false);
  valveToggle3->setVariant(CardVariant::INFO);
  valveToggle3->onChange = [](bool value)
  {
    digitalWrite(valve3, value ? HIGH : LOW);
  };
  StatusToggleCard *valveToggle4 = dashboard.addStatusToggleCard("valve4", "Valve 4", "Open", "Closed", StatusIcon::RAIN, StatusIcon::CLOUD, false);
  valveToggle4->setVariant(CardVariant::INFO);
  valveToggle4->onChange = [](bool value)
  {
    digitalWrite(valve4, value ? HIGH : LOW);
  };

  ChartCard *moistChart1 = dashboard.addChartCard("moistChart1", "Moisture 1", "%", ChartType::LINE, 48);
  moistChart1->setSize(2, 1);
  ChartCard *moistChart2 = dashboard.addChartCard("moistChart2", "Moisture 2", "%", ChartType::LINE, 48);
  moistChart2->setSize(2, 1);
  ChartCard *moistChart3 = dashboard.addChartCard("moistChart3", "Moisture 3", "%", ChartType::LINE, 48);
  moistChart3->setSize(2, 1);
  ChartCard *moistChart4 = dashboard.addChartCard("moistChart4", "Moisture 4", "%", ChartType::LINE, 48);
  moistChart4->setSize(2, 1);

  dashboard.addStatCard("flow1", "Flow 1", "0", "l/min");
  dashboard.addStatCard("flow2", "Flow 2", "0", "l/min");
  dashboard.addStatCard("flow3", "Flow 3", "0", "l/min");
  dashboard.addStatCard("flow4", "Flow 4", "0", "l/min");

  dashboard.addGroup("stuff", "Stuff", {"valveDropdown", "durationSlider", "doWater", "restart"});
  dashboard.addGroup("valve", "Valve States", {"valve1", "valve2", "valve3", "valve4"});
  dashboard.addGroup("moistureChart", "Soil Moisture", {"moistChart1", "moistChart2", "moistChart3", "moistChart4"});
  dashboard.addGroup("flow", "Water Flow", {"flow1", "flow2", "flow3", "flow4"});
}

void calculateFlow()
{
  flow_l_min_1 = flowFrequency1 / 7.5;
  flow_l_min_2 = flowFrequency2 / 7.5;
  flow_l_min_3 = flowFrequency3 / 7.5;
  flow_l_min_4 = flowFrequency4 / 7.5;
  flowFrequency1 = 0;
  flowFrequency2 = 0;
  flowFrequency3 = 0;
  flowFrequency4 = 0;

  /*
  openValveIfSoilDry(moist1, valve1);
  openValveIfSoilDry(moist2, valve2);
  openValveIfSoilDry(moist3, valve3);
  openValveIfSoilDry(moist4, valve4);
*/
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
    dashboard.updateStatusToggleCard(valveCard, digitalRead(valvePin));
}

void updateDashboard()
{
  updateValveState(valve1, "valve1");
  updateValveState(valve2, "valve2");
  updateValveState(valve3, "valve3");
  updateValveState(valve4, "valve4");

  calculateFlow();
  dashboard.updateStatCard("flow1", String(flow_l_min_1));
  dashboard.updateStatCard("flow2", String(flow_l_min_2));
  dashboard.updateStatCard("flow3", String(flow_l_min_3));
  dashboard.updateStatCard("flow4", String(flow_l_min_4));
}

void updateMoistureCharts()
{
  dashboard.updateChartCard("moistChart1", readMoistValue(moist1));
  dashboard.updateChartCard("moistChart2", readMoistValue(moist2));
  dashboard.updateChartCard("moistChart3", readMoistValue(moist3));
  dashboard.updateChartCard("moistChart4", readMoistValue(moist4));
}

void flowCalculationTickerCallback()
{
  doDashboardUpdate = true;
}

void moistureChartTickerCallback()
{
  doMoistureChartUpdate = true;
}

void setup()
{
  Serial.begin(9600);
  setupPins();
  setupServer();
  setupDashboard();

  flowCalculationTicker.attach_ms(250, flowCalculationTickerCallback);
  moistureChartTicker.attach(3600, moistureChartTickerCallback);
  calculateFlow();
  updateMoistureCharts();
  dashboard.updateDropdownCard("valveDropdown", "0");
  dashboard.updateSliderCard("durationSlider", 1);

  sei();
}

void loop()
{
  dashboard.loop();

  if (doDashboardUpdate)
  {
    noInterrupts();
    doDashboardUpdate = false;
    interrupts();
    updateDashboard();
  }

  if (doMoistureChartUpdate)
  {
    noInterrupts();
    doMoistureChartUpdate = false;
    interrupts();
    updateMoistureCharts();
  }

  if (doWatering && millis() >= wateringEndTime)
  {
    dashboard.updateButtonCard("doWater", true);
    digitalWrite(valves[selectedValve], LOW);
    doWatering = false;
  }

  delay(50);
}