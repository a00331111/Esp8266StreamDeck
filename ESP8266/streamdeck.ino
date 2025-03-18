#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <EEPROM.h>

// 配网信息存储结构
struct Config {
  char ssid[32];
  char password[64];
  char serverIp[16];
};

Config config;
bool isConfigured = false;

ESP8266WebServer server(80);

// 矩阵键盘引脚定义
const byte ROWS = 4; // 行数
const byte COLS = 4; // 列数

byte rowPins[ROWS] = {5, 4, 0, 14};  // 行引脚：D1, D2, D3, D5
byte colPins[COLS] = {12, 13, 15, 2}; // 列引脚：D6, D7, D8, D4

// 加载配置
void loadConfig() {
  EEPROM.begin(sizeof(Config));
  EEPROM.get(0, config);
  EEPROM.end();

  // 检查配置是否有效
  if (String(config.ssid).length() > 0 && String(config.serverIp).length() > 0) {
    isConfigured = true;
  } else {
    isConfigured = false;
  }
}

// 保存配置
void saveConfig() {
  EEPROM.begin(sizeof(Config));
  EEPROM.put(0, config);
  EEPROM.commit();
  EEPROM.end();
}

// 配网页面
void handleRoot() {
  String html = R"(
    <!DOCTYPE html>
    <html>
    <head><title>ESP8266 Config</title></head>
    <body>
      <h1>ESP8266 Configuration</h1>
      <form action="/save" method="POST">
        <label for="ssid">Wi-Fi SSID:</label><br>
        <input type="text" id="ssid" name="ssid" value="%SSID%" required><br><br>
        <label for="password">Wi-Fi Password:</label><br>
        <input type="password" id="password" name="password" value="%PASSWORD%" required><br><br>
        <label for="serverIp">Computer IP:</label><br>
        <input type="text" id="serverIp" name="serverIp" value="%SERVERIP%" required><br><br>
        <button type="submit">Save</button>
      </form>
    </body>
    </html>
  )";

  // 替换占位符为实际配置值
  html.replace("%SSID%", String(config.ssid));
  html.replace("%PASSWORD%", String(config.password));
  html.replace("%SERVERIP%", String(config.serverIp));

  server.send(200, "text/html", html);
}

// 保存配置并重启
void handleSave() {
  strncpy(config.ssid, server.arg("ssid").c_str(), sizeof(config.ssid));
  strncpy(config.password, server.arg("password").c_str(), sizeof(config.password));
  strncpy(config.serverIp, server.arg("serverIp").c_str(), sizeof(config.serverIp));
  saveConfig();
  server.send(200, "text/plain", "Configuration saved! Restarting...");
  delay(1000);
  ESP.restart();
}

// 发送按键触发请求
void sendKeyPress(int keyId) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    String url = "http://" + String(config.serverIp) + ":5000/trigger?key=" + String(keyId);
    http.begin(client, url);
    int httpCode = http.GET();
    http.end();
  }
}

void setup() {
  Serial.begin(115200);

  // 初始化矩阵键盘
  for (int i = 0; i < ROWS; i++) {
    pinMode(rowPins[i], INPUT_PULLUP);
  }
  for (int j = 0; j < COLS; j++) {
    pinMode(colPins[j], OUTPUT);
    digitalWrite(colPins[j], HIGH);
  }

  // 加载配置
  loadConfig();

  if (isConfigured) {
    // 已配置，连接到 Wi-Fi
    WiFi.begin(config.ssid, config.password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");
    Serial.println("IP: " + WiFi.localIP().toString());
  } else {
    // 未配置，进入配网模式
    WiFi.softAP("ESP8266-Config");
    Serial.println("AP Mode: ESP8266-Config");
    Serial.println("IP: " + WiFi.softAPIP().toString());

    server.on("/", handleRoot);
    server.on("/save", handleSave);
    server.begin();
    Serial.println("HTTP server started");
  }
}

void loop() {
  if (isConfigured) {
    // 运行模式：扫描矩阵键盘
    for (int col = 0; col < COLS; col++) {
      digitalWrite(colPins[col], LOW);
      for (int row = 0; row < ROWS; row++) {
        if (digitalRead(rowPins[row]) == LOW) {
          int keyIndex = row * COLS + col;
          sendKeyPress(keyIndex);
          delay(200); // 防抖
          while (digitalRead(rowPins[row]) == LOW); // 等待按键释放
        }
      }
      digitalWrite(colPins[col], HIGH);
    }
  } else {
    // 配网模式：处理 Web 请求
    server.handleClient();
  }
}