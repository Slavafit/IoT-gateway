#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>

#define RX_PIN 13 // D7
#define TX_PIN 5  // D1
#define LED_PIN 2 // D4

SoftwareSerial scaleSerial(RX_PIN, TX_PIN);
ESP8266WebServer server(80);

String rawData = "";
float currentWeight = 0.0;

// ==========================================
// СТРУКТУРА НАСТРОЕК ДЛЯ EEPROM
// ==========================================
struct NetConfig {
  char magic[4];  // Метка наличия сохранения (CFG)
  char ssid[32];  // Имя Wi-Fi
  char pass[64];  // Пароль Wi-Fi
  uint8_t ip[4];  // IP-адрес (4 блока)
};

NetConfig cfg;

// HTML-шаблон для браузера
const char* htmlPage = R"rawliteral(
<!DOCTYPE html><html lang="ru"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Настройки весов</title>
<style>
  body { font-family: Arial, sans-serif; background: #e9ecef; display: flex; justify-content: center; padding-top: 50px; }
  .card { background: #fff; padding: 30px; border-radius: 12px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 100%; max-width: 350px; }
  h2 { text-align: center; color: #333; margin-top: 0; }
  label { font-weight: bold; font-size: 14px; color: #555; }
  input { width: 100%; padding: 12px; margin: 8px 0 20px 0; border: 1px solid #ccc; border-radius: 6px; box-sizing: border-box; font-size: 16px; }
  button { background: #007bff; color: white; border: none; padding: 14px; border-radius: 6px; cursor: pointer; width: 100%; font-size: 16px; font-weight: bold; }
  button:hover { background: #0056b3; }
</style></head><body>
  <div class="card">
    <h2>Wi-Fi и IP весов</h2>
    <form action="/save" method="POST">
      <label>Имя сети (SSID):</label>
      <input type="text" name="ssid" value="%SSID%" required>
      <label>Пароль:</label>
      <input type="text" name="pass" value="%PASS%">
      <label>Статический IP:</label>
      <input type="text" name="ip" value="%IP%" placeholder="192.168.1.150" required>
      <button type="submit">Сохранить и перезагрузить</button>
    </form>
  </div>
</body></html>
)rawliteral";

// Загрузка настроек из памяти
void loadConfig() {
  EEPROM.begin(512);
  EEPROM.get(0, cfg);
  
  // Если метка не совпадает, память пуста. Записываем дефолтные значения.
  if (String(cfg.magic) != "CFG") {
    strcpy(cfg.magic, "CFG");
    strcpy(cfg.ssid, "MyNetwork");
    strcpy(cfg.pass, "MyPassword");
    cfg.ip[0] = 192; cfg.ip[1] = 168; cfg.ip[2] = 1; cfg.ip[3] = 150;
    EEPROM.put(0, cfg);
    EEPROM.commit();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  scaleSerial.begin(2400, SWSERIAL_8N1, RX_PIN, TX_PIN, false); 

  loadConfig(); // Читаем сохраненные настройки

  // Настройка статического IP (шлюз вычисляется автоматически: 192.168.X.1)
  IPAddress local_IP(cfg.ip[0], cfg.ip[1], cfg.ip[2], cfg.ip[3]);
  IPAddress gateway(cfg.ip[0], cfg.ip[1], cfg.ip[2], 1); 
  IPAddress subnet(255, 255, 255, 0);

  WiFi.hostname("Wireless-Scales");
  WiFi.config(local_IP, gateway, subnet);
  
  Serial.println("\nПодключение к " + String(cfg.ssid) + "...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.ssid, cfg.pass);

  // Пытаемся подключиться 20 секунд
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) { 
    digitalWrite(LED_PIN, (millis() / 200) % 2);
    delay(500); 
    Serial.print(".");
    tries++;
  }

  // ЕСЛИ НЕ ПОДКЛЮЧИЛИСЬ — СОЗДАЕМ СВОЮ СЕТЬ
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nОшибка Wi-Fi! Запускаем точку настройки.");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("SmartScales-Setup"); // Сеть без пароля
    Serial.println("Подключитесь к Wi-Fi: SmartScales-Setup");
    Serial.println("Откройте в браузере: http://192.168.4.1/config");
  } else {
    Serial.println("\nПодключено! IP: " + WiFi.localIP().toString());
  }

  // ==========================================
  // ВЕБ-СЕРВЕР (ENDPOINTS)
  // ==========================================

  // 1. JSON для вашего приложения Kotlin
  server.on("/", []() {
    String json = "{\"weight\":" + String(currentWeight, 2) + ",\"raw\":\"" + rawData + "\",\"unit\":\"kg\"}";
    server.send(200, "application/json", json);
  });

  // 2. HTML-страница настроек для браузера
  server.on("/config", []() {
    String page = String(htmlPage);
    page.replace("%SSID%", String(cfg.ssid));
    page.replace("%PASS%", String(cfg.pass));
    String ipStr = String(cfg.ip[0]) + "." + String(cfg.ip[1]) + "." + String(cfg.ip[2]) + "." + String(cfg.ip[3]);
    page.replace("%IP%", ipStr);
    server.send(200, "text/html", page);
  });

  // 3. Обработка сохранения настроек
  server.on("/save", HTTP_POST, []() {
    if (server.hasArg("ssid")) strcpy(cfg.ssid, server.arg("ssid").c_str());
    if (server.hasArg("pass")) strcpy(cfg.pass, server.arg("pass").c_str());
    
    if (server.hasArg("ip")) {
      int parts[4];
      if (sscanf(server.arg("ip").c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]) == 4) {
        for(int i=0; i<4; i++) cfg.ip[i] = (uint8_t)parts[i];
      }
    }
    
    EEPROM.put(0, cfg);
    EEPROM.commit();
    server.send(200, "text/html", "<h2 style='font-family:sans-serif;text-align:center;margin-top:50px;'>Сохранено! Весы перезагружаются...</h2>");
    delay(1000);
    ESP.restart(); // Перезагружаем плату для применения настроек
  });

  server.begin();
}

void loop() {
  server.handleClient();
  
  // Маячок: работает ли сеть (горит постоянно = AP Mode, мигает раз в 3 сек = Подключено)
  if (WiFi.getMode() == WIFI_AP) {
    digitalWrite(LED_PIN, LOW); // Светится постоянно, прося настройки
  } else if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_PIN, (millis() % 3000 < 50) ? LOW : HIGH); 
  }

  // ПАРСЕР ВЕСОВ (Переворот строки)
  if (scaleSerial.available()) {
    char c = scaleSerial.read();
    digitalWrite(LED_PIN, LOW); 

    if (c == ' ' || c == 13 || c == 10 || c == '=') {
      if (rawData.length() > 0) { 
        String reversed = "";
        for (int i = rawData.length() - 1; i >= 0; i--) reversed += rawData[i];
        currentWeight = reversed.toFloat();
      }
      rawData = ""; 
      digitalWrite(LED_PIN, HIGH);
    } 
    else if ((c >= '0' && c <= '9') || c == '.') {
      rawData += c;
    }
  }
}