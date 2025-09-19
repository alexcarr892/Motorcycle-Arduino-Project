#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <SPIFFS.h>
#include <vector>
#include <time.h>

const char* ssid = "S23";
const char* password = "arduinopass";

#define SDA_PIN 11
#define SCL_PIN 12
#define TCA9548A_ADDR 0x70

LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_MLX90614 mlx1 = Adafruit_MLX90614();
Adafruit_MLX90614 mlx2 = Adafruit_MLX90614();

WebServer server(80);

std::vector<float> frontTemps;
std::vector<float> rearTemps;
std::vector<unsigned long> timestampLog;
unsigned long lastGraphUpdate = 0;
const unsigned long graphInterval = 15000;  // 15 seconds
bool captureData = true;

String logFilename;

void tcaSelect(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(10);
}

void updateLCD(float front, float rear) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Front: ");
  lcd.print(front, 1);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Rear:  ");
  lcd.print(rear, 1);
  lcd.print("C");
}

String generateGraphHTML() {
  float frontHigh = -1000, frontLow = 1000, rearHigh = -1000, rearLow = 1000;
  for (size_t i = 0; i < frontTemps.size(); ++i) {
    if (frontTemps[i] > frontHigh) frontHigh = frontTemps[i];
    if (frontTemps[i] < frontLow) frontLow = frontTemps[i];
    if (rearTemps[i] > rearHigh) rearHigh = rearTemps[i];
    if (rearTemps[i] < rearLow) rearLow = rearTemps[i];
  }

  String html = "<html><head><meta http-equiv='refresh' content='10'><script src='https://cdn.jsdelivr.net/npm/chart.js'></script></head><body>";
  html += "<h2>Temperature Over Time</h2><canvas id='tempChart' width='400' height='200'></canvas><script>";

  html += "const ctx = document.getElementById('tempChart').getContext('2d');";
  html += "const tempChart = new Chart(ctx, { type: 'line', data: { labels: [";

  for (size_t i = 0; i < timestampLog.size(); ++i) {
    html += "'" + String(timestampLog[i] / 1000) + "s'";
    if (i < timestampLog.size() - 1) html += ", ";
  }

  html += "], datasets: [";
  html += "{ label: 'Front (°C)', data: [";
  for (size_t i = 0; i < frontTemps.size(); ++i) {
    html += String(frontTemps[i]);
    if (i < frontTemps.size() - 1) html += ", ";
  }
  html += "], borderColor: 'rgba(255, 99, 132, 1)', borderWidth: 2, fill: false },";

  html += "{ label: 'Rear (°C)', data: [";
  for (size_t i = 0; i < rearTemps.size(); ++i) {
    html += String(rearTemps[i]);
    if (i < rearTemps.size() - 1) html += ", ";
  }
  html += "] , borderColor: 'rgba(54, 162, 235, 1)', borderWidth: 2, fill: false }] }, options: { scales: { y: { beginAtZero: false }}} });";
  html += "</script>";

  html += "<h3>Peak Temps</h3>";
  html += "<p>Front High: " + String(frontHigh) + " C, Low: " + String(frontLow) + " C</p>";
  html += "<p>Rear High: " + String(rearHigh) + " C, Low: " + String(rearLow) + " C</p>";
  html += "<form action='/toggle' method='POST'><button type='submit'>";
  html += captureData ? "Stop Capture" : "Start Capture";
  html += "</button></form>";
  html += "<a href='/download'>Download Log</a>";
  html += "</body></html>";

  return html;
}

void handleRoot() {
  server.send(200, "text/html", generateGraphHTML());
}

void handleToggle() {
  captureData = !captureData;
  if (captureData) {
    frontTemps.clear();
    rearTemps.clear();
    timestampLog.clear();
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDownload() {
  File file = SPIFFS.open(logFilename.c_str(), "r");
  if (!file || file.isDirectory()) {
    server.send(404, "text/plain", "Log file not found");
    return;
  }
  server.streamFile(file, "text/plain");
  file.close();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  delay(1000);

  SPIFFS.begin(true);
  File root = SPIFFS.open("/");
  while (File file = root.openNextFile()) {
    SPIFFS.remove(file.name());
  }

  struct tm timeinfo;
  time_t now = time(nullptr);
  localtime_r(&now, &timeinfo);
  char filename[32];
  snprintf(filename, sizeof(filename), "/log_%04d%02d%02d_%02d%02d%02d.csv",
           timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
           timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  logFilename = String(filename);

  File logFile = SPIFFS.open(logFilename.c_str(), FILE_WRITE);
  if (logFile) {
    logFile.println("Time (ms), Front Temp (°C), Rear Temp (°C)");
    logFile.close();
  }

  tcaSelect(2);
  bool sensor1Found = mlx1.begin();
  tcaSelect(5);
  bool sensor2Found = mlx2.begin();

  if (!sensor1Found || !sensor2Found) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sensor Fail");
    while (1);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP:");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  delay(5000);

  server.on("/", handleRoot);
  server.on("/toggle", HTTP_POST, handleToggle);
  server.on("/download", HTTP_GET, handleDownload);
  server.begin();
}

void loop() {
  tcaSelect(2);
  float front = mlx1.readObjectTempC();
  tcaSelect(5);
  float rear = mlx2.readObjectTempC();

  updateLCD(front, rear);
  if (captureData) {
    unsigned long now = millis();
    if (now - lastGraphUpdate >= graphInterval) {
      frontTemps.push_back(front);
      rearTemps.push_back(rear);
      timestampLog.push_back(now);
      File logFile = SPIFFS.open(logFilename.c_str(), FILE_APPEND);
      if (logFile) {
        logFile.printf("%lu,%.2f,%.2f\n", now, front, rear);
        logFile.close();
      }
      lastGraphUpdate = now;
    }
  }
  server.handleClient();
  delay(1000);
}
