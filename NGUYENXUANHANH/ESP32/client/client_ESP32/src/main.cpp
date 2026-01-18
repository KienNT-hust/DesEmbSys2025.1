/*
#include <Arduino.h>
#include<WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include<HTTPClient.h>
#define DHTPIN 4
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
const char *ssid = "Iphone của Hạnh";
const char *password = "01052004";
const char *url = "http://10.136.192.207:3000";
DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(9600);
 WiFi.begin(ssid,password);
 dht.begin();
 while (WiFi.status() != WL_CONNECTED)
 {
  Serial.print("..");
  delay(200);
 }
 Serial.println("Connected...good");
}

void loop() {
 // Doc nhiet do, do am
 float temp = 0;
 float hum = 0;
 if(dht.readTemperature() && dht.readHumidity()){
  temp = dht.readTemperature();
  hum = dht.readHumidity();
 }
 //Serial.println("Nhiet do", temp);
Serial.print("Nhiet do: ");
Serial.println(temp);
Serial.print("do am: ");
Serial.println(hum);
// gui len server bang phuong thuc call API post
if (WiFi.status() == WL_CONNECTED)
{
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type","application/json");
  String jsonData = "{\"temp\":\""+String(temp) +"\",\"hum\":\""+String(hum) +"\"}";
  //call 1 cai API
  int res = http.POST(jsonData);
  if(res>0){
    Serial.println("Gui thanh cong");
    Serial.print("HTTP Code: ");
Serial.println(res); // Để xem nó là 200, 404 hay -1
  }else{
    Serial.println("Gui that bai");
   Serial.print("HTTP Code: ");
Serial.println(res); // Để xem nó là 200, 404 hay -1
  }
  http.end();
}
delay(5000);
}
*/
/*
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ===== LoRa pins (ESP32 DOIT + RA-01/RA-02) =====
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    27   // đổi thành 5 nếu bạn cắm NSS=GPIO5
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_FREQ  433E6

SPIClass spiLoRa(VSPI);

// ===== WiFi + Server =====
const char* WIFI_SSID = "Iphone của Hạnh";
const char* WIFI_PASS = "01052004";

// IP MÁY CHẠY NODE.JS (cùng mạng WiFi với ESP32)
const char* SERVER_URL = "http://10.136.192.207:3000/api/uplink";

void ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi OK, IP=");
  Serial.println(WiFi.localIP());
}

void postToServer(const String& raw, long rssi, float snr) {
  ensureWiFi();

  HTTPClient http;
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  // JSON tối giản (không cần ArduinoJson)
  String json = "{";
  json += "\"raw\":\"" + raw + "\",";
  json += "\"rssi\":" + String(rssi) + ",";
  json += "\"snr\":" + String(snr, 2);
  json += "}";

  int code = http.POST(json);
  Serial.print("POST code="); Serial.println(code);
  http.end();
}

void setup() {
  Serial.begin(9600);
  delay(500);

  ensureWiFi();

  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(spiLoRa);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed!");
    while (1) delay(1000);
  }

  // phải giống STM32
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  Serial.println("Gateway ready: LoRa RX -> HTTP POST");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String payload;
    payload.reserve(packetSize);
    while (LoRa.available()) payload += (char)LoRa.read();

    long rssi = LoRa.packetRssi();
    float snr = LoRa.packetSnr();

    Serial.print("RX: ");
    Serial.print(payload);
    Serial.print(" | RSSI=");
    Serial.print(rssi);
    Serial.print(" dBm | SNR=");
    Serial.println(snr, 2);

    postToServer(payload, rssi, snr);
  }
}
*/
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

// ================== CONFIG ==================
#define WIFI_ENABLE 1      // 1 = bật WiFi + POST server, 0 = chỉ test LoRa
#define USE_GACK    1      // 1 = Gateway gửi GACK lại Relay khi nhận TYPE=FWD
#define WIFI_TIMEOUT_MS 20000

// ===== LoRa pins (ESP32 DOIT + RA-01/RA-02) =====
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    27      // đúng như bạn cắm
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_FREQ  433E6

SPIClass spiLoRa(VSPI);

// ===== WiFi + Server =====
const char* WIFI_SSID = "Iphone của Hạnh";
const char* WIFI_PASS = "01052004";
const char* SERVER_URL = "http://10.136.192.207:3000/api/uplink";

// ================== SMALL UTILS ==================
static const char* wlStatusName(wl_status_t s) {
  switch (s) {
    case WL_IDLE_STATUS:     return "IDLE";
    case WL_NO_SSID_AVAIL:   return "NO_SSID";
    case WL_SCAN_COMPLETED:  return "SCAN_DONE";
    case WL_CONNECTED:       return "CONNECTED";
    case WL_CONNECT_FAILED:  return "CONNECT_FAILED";
    case WL_CONNECTION_LOST: return "CONNECTION_LOST";
    case WL_DISCONNECTED:    return "DISCONNECTED";
    default:                 return "UNKNOWN";
  }
}

// ===== parse key=value; in payload =====
static bool kvGet(const String& s, const char* key, String& out) {
  String k = String(key) + "=";
  int p = s.indexOf(k);
  if (p < 0) return false;
  p += k.length();
  int q = s.indexOf(';', p);
  if (q < 0) q = s.length();
  out = s.substring(p, q);
  out.trim();
  return out.length() > 0;
}

static String jsonEscape(const String& s) {
  String out;
  out.reserve(s.length() + 16);
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '\"': out += "\\\""; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if ((uint8_t)c >= 0x20) out += c;
        break;
    }
  }
  return out;
}

// nếu payload dạng TYPE=FWD;REL=...;SRC=... thì đổi SRC->ID để server parse
static String normalizeForServer(String raw) {
  if (raw.indexOf("ID=") < 0 && raw.indexOf("SRC=") >= 0) {
    raw.replace("SRC=", "ID=");
  }
  return raw;
}

// ================== WiFi EVENT (có reason thật) ==================
static void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
    Serial.print("[GW][WIFI] DISCONNECTED, reason=");
    Serial.println(info.wifi_sta_disconnected.reason); // ✅ reason thật
  } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
    Serial.println("[GW][WIFI] CONNECTED to AP");
  } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    Serial.print("[GW][WIFI] GOT IP: ");
    Serial.println(WiFi.localIP());
  }
}

// ================== WiFi / HTTP ==================
static bool ensureWiFi(uint32_t timeoutMs = WIFI_TIMEOUT_MS) {
#if WIFI_ENABLE == 0
  return false;
#else
  if (WiFi.status() == WL_CONNECTED) return true;

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);

  Serial.print("[GW][WIFI] connecting to: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - t0 > timeoutMs) {
      Serial.print("\n[GW][WIFI] timeout | status=");
      Serial.print((int)WiFi.status());
      Serial.print(" (");
      Serial.print(wlStatusName(WiFi.status()));
      Serial.println(")");
      return false;
    }
  }

  Serial.print("\n[GW][WIFI] OK, IP=");
  Serial.println(WiFi.localIP());
  return true;
#endif
}

static int postToServer(const String& rawNormalized, long rssi, float snr) {
#if WIFI_ENABLE == 0
  (void)rawNormalized; (void)rssi; (void)snr;
  return 0;
#else
  if (!ensureWiFi()) {
    Serial.println("[GW][HTTP] skip POST (WiFi not connected)");
    return -100;
  }

  HTTPClient http;
  http.setTimeout(6000);
  http.setReuse(false);                 // hạn chế lỗi keep-alive lằng nhằng
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "close");

  String json = "{";
  json += "\"raw\":\"" + jsonEscape(rawNormalized) + "\",";
  json += "\"rssi\":" + String(rssi) + ",";
  json += "\"snr\":"  + String(snr, 2);
  json += "}";

  int code = http.POST(json);

  Serial.print("[GW][HTTP] POST code=");
  Serial.println(code);

  if (code > 0) {
    String resp = http.getString();
    if (resp.length()) {
      Serial.print("[GW][HTTP] resp: ");
      Serial.println(resp);
    }
  } else {
    Serial.print("[GW][HTTP] error: ");
    Serial.println(http.errorToString(code));
  }

  http.end();
  return code;
#endif
}

// ================== LoRa GACK ==================
static void sendGack(const String& relStr, const String& srcStr, const String& seqStr) {
#if USE_GACK == 0
  (void)relStr; (void)srcStr; (void)seqStr;
  return;
#else
  String ack = "TYPE=GACK;";
  if (relStr.length()) ack += "REL=" + relStr + ";";
  if (srcStr.length()) ack += "SRC=" + srcStr + ";";
  if (seqStr.length()) ack += "SEQ=" + seqStr;

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(ack);
  LoRa.endPacket();
  LoRa.receive();

  Serial.print("[GW] GACK TX: ");
  Serial.println(ack);
#endif
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(9600);
  delay(500);

#if WIFI_ENABLE == 1
  WiFi.onEvent(onWiFiEvent);
  ensureWiFi(); // thử connect ngay từ đầu
#else
  Serial.println("[GW] WIFI_ENABLE=0 (LoRa only)");
#endif

  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(spiLoRa);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("[GW] LoRa init failed!");
    while (1) delay(1000);
  }

  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();

  Serial.println("[GW] READY: LoRa RX (from RELAY) -> (opt GACK) -> (opt HTTP POST)");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  String payload;
  payload.reserve(packetSize);
  while (LoRa.available()) payload += (char)LoRa.read();

  long rssi = LoRa.packetRssi();
  float snr = LoRa.packetSnr();

  Serial.print("[GW] RX: ");
  Serial.print(payload);
  Serial.print(" | RSSI=");
  Serial.print(rssi);
  Serial.print(" dBm | SNR=");
  Serial.println(snr, 2);

  // nếu là forward từ relay: TYPE=FWD;REL=..;SRC=..;SEQ=..;...
  if (payload.indexOf("TYPE=FWD") >= 0) {
    String relStr, srcStr, seqStr;
    kvGet(payload, "REL", relStr);
    kvGet(payload, "SRC", srcStr);
    kvGet(payload, "SEQ", seqStr);
    sendGack(relStr, srcStr, seqStr);
  }

  String rawForServer = normalizeForServer(payload);
  postToServer(rawForServer, rssi, snr);
}
