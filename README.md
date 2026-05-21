# Smart Scales IoT Gateway (ESP8266)

An ESP8266-based hardware gateway that reads weight data from a commercial weighing terminal via UART, parses the inverted protocol, and serves it as a JSON API over Wi-Fi for Android (Kotlin) applications.

---

## Features
- **Smart Protocol Parsing**: Automatically handles inverted data packets (Little-Endian string format) from the scale terminal (e.g., transforms raw `41.1000` into `1.14` kg).
- **Web Configurator**: Built-in HTML panel accessible at `/config` to change Wi-Fi credentials and static IP without reflashing.
- **EEPROM Storage**: Network settings are preserved across power cycles.
- **Captive Fallback**: If the configured Wi-Fi is unavailable, the device automatically creates an open Access Point (`SmartScales-Setup`) for troubleshooting.
- **LED Diagnostics**: Smart blinking patterns for Wi-Fi searching, AP mode, active connection, and data RX bursts.

## Hardware Setup
- **Microcontroller**: ESP8266 (Wemos / Lolin D1 Mini)
- **Interface**: SoftwareSerial on Pins D7 (RX) and D1 (TX)
- **Baud Rate**: 2400 baud, 8N1
- **Voltage Protection**: A resistor voltage divider ($1\text{ k}\Omega / 2\text{ k}\Omega$) must be used on the ESP's RX pin to safe-drop the scale's 5V TX logic down to the ESP-friendly 3.3V. Common GND is required.

## API Endpoint
### GET `/`
Returns the current stabilized weight.
**Response (application/json):**
```json
{
  "weight": 1.140,
  "raw": "41.1000",
  "unit": "kg"
}
# IoT-gateway
Разработка аппаратно-программного комплекса на базе микроконтроллера ESP8266 для считывания данных с весового терминала по интерфейсу UART, их парсинга и передачи в мобильное Android-приложение по Wi-Fi (HTTP/JSON).

Техническое задание: IoT-шлюз для весового терминала
1. Общее описание проекта
Разработка аппаратно-программного комплекса на базе микроконтроллера ESP8266 для считывания данных с весового терминала по интерфейсу UART, их парсинга и передачи в мобильное Android-приложение по Wi-Fi (HTTP/JSON).

2. Аппаратная часть (Hardware)

Микроконтроллер: ESP8266 (плата форм-фактора Wemos / Lolin D1 Mini).

Подключение к весам: Интерфейс UART (TX весов -> RX ESP8266).

Защита порта: Делитель напряжения (резисторы 1 кОм и 2 кОм) для согласования логических уровней 5V (весы) и 3.3V (ESP). Общая земля (GND).

Световая индикация: Встроенный синий светодиод на пине D4 (GPIO2).

3. Программная часть ESP8266 (Firmware)

Параметры UART: 2400 baud, 8N1, чтение через программный порт (SoftwareSerial).

Логика парсера данных:

Устройство принимает «сырые» пакеты, где цифры веса передаются в обратном порядке (Little-Endian формат для строк). Пример: вес 1.14 кг передается как 41.1000.

Алгоритм программно «переворачивает» строку и преобразует её в десятичное число (float).

Сетевое поведение и настройки:

Данные сети (SSID, Пароль, Статический IP) хранятся в энергонезависимой памяти (EEPROM).

Fallback-режим: Если подключение к рабочему роутеру не удалось (ошибка пароля или нет сети), через 20 секунд ESP поднимает собственную точку доступа SmartScales-Setup (без пароля).

Web-конфигуратор: По адресу 192.168.4.1/config (в режиме AP) или [Текущий_IP]/config (в рабочей сети) доступна HTML-страница для смены настроек сети.

Интерфейс отдачи данных (API):

Метод: HTTP GET на корень (/).

Формат ответа: {"weight": 1.14, "raw": "41.1000", "unit": "kg"}

Паттерны индикации (LED):

Поиск Wi-Fi: Быстрое мигание.

Режим настройки (AP Mode): Горит постоянно.

Штатная работа (Подключено): Короткая вспышка-маячок раз в 3 секунды.

Прием данных: Мгновенная вспышка при успешном чтении пакета от весов.

4. Клиентская часть (Android / Kotlin)

Сетевое взаимодействие: Отправка GET-запроса на статический IP весов, десериализация JSON-ответа, вывод поля weight в UI.

Требования к пользовательскому интерфейсу (UI):

Использование Material Design 3.

Второстепенные кнопки (например, расчет объема "V", сканер) реализованы в стиле TextButton (без заливки и обводки, с эффектом ripple при нажатии).

Единый размер кликабельной зоны: 48x48 dp (размер иконок 24dp).

Выравнивание в строку: Контейнер LinearLayout (horizontal) с отключенным выравниванием по тексту (android:baselineAligned="false") и гравитацией по нижнему краю или центру (gravity="bottom" или center_vertical).
