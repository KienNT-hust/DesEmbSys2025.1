/*
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// ===== LoRa pins (ESP32 VSPI) =====
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    27   // đổi thành 5 nếu bạn dùng NSS=GPIO5
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_FREQ  433E6

SPIClass spiLoRa(VSPI);

// ===== TDMA params (test nhanh) =====
// Khi demo xong, đổi PERIOD_MS = 300000 (5 phút)
static const uint32_t PERIOD_MS = 30000;   // 30s cho test nhanh
static const uint32_t SLOT_MS   = 2000;    // 2s/slot
static const uint8_t  NODES     = 3;       // hỗ trợ tới 3 node
static uint16_t SFID = 0;

bool readPacket(String &out) {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return false;
  out = "";
  while (LoRa.available()) out += (char)LoRa.read();
  return true;
}

bool getKV(const String &s, const String &key, String &out) {
  int p = s.indexOf(key + "=");
  if (p < 0) return false;
  p += key.length() + 1;
  int q = s.indexOf(';', p);
  if (q < 0) q = s.length();
  out = s.substring(p, q);
  out.trim();
  return out.length() > 0;
}

void sendBeacon() {
  char b[80];
  snprintf(b, sizeof(b), "TYPE=BEACON;SFID=%u;PER=%lu;SLOT=%lu;N=%u",
           SFID++, (unsigned long)(PERIOD_MS/1000), (unsigned long)SLOT_MS, (unsigned)NODES);

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(b);
  LoRa.endPacket();
  LoRa.receive();

  Serial.print("[BEACON TX] ");
  Serial.println(b);
}

void sendAck(uint8_t src, uint16_t seq) {
  char a[50];
  snprintf(a, sizeof(a), "TYPE=ACK;SRC=%u;SEQ=%u", src, seq);

  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(a);
  LoRa.endPacket();
  LoRa.receive();

  Serial.print("  [ACK TX] ");
  Serial.println(a);
}

void setup() {
  Serial.begin(9600);
  delay(500);

  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(spiLoRa);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed!");
    while (1) delay(1000);
  }

  // phải KHỚP với STM32
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  LoRa.receive();
  Serial.println("ESP32 RELAY ready: BEACON + TDMA RX + ACK");
}

void loop() {
  uint32_t t0 = millis();

  // 1) Phát BEACON đầu chu kỳ
  sendBeacon();

  // 2) TDMA receive theo slot (đơn giản: 1 slot/1 node)
  for (uint8_t slot = 0; slot < NODES; slot++) {
    uint32_t slotStart = t0 + 200 + (uint32_t)slot * SLOT_MS; // guard 200ms
    while ((int32_t)(millis() - slotStart) < 0) delay(5);

    uint32_t slotEnd = slotStart + SLOT_MS - 200;
    Serial.print("[SLOT "); Serial.print(slot+1); Serial.println("] listening...");

    while ((int32_t)(millis() - slotEnd) < 0) {
      String rx;
      if (readPacket(rx)) {
        // chỉ xử lý DATA
        if (rx.indexOf("TYPE=DATA") >= 0) {
          String srcStr, seqStr;
          if (getKV(rx, "SRC", srcStr) && getKV(rx, "SEQ", seqStr)) {
            uint8_t src = (uint8_t)srcStr.toInt();
            uint16_t sq = (uint16_t)seqStr.toInt();

            Serial.print("  [RX] "); Serial.print(rx);
            Serial.print(" | RSSI="); Serial.print(LoRa.packetRssi());
            Serial.print(" | SNR="); Serial.println(LoRa.packetSnr(), 2);

            // ACK ngay lập tức
            sendAck(src, sq);
          } else {
            Serial.print("  [RX] "); Serial.println(rx);
          }
        }
      }
      delay(5);
    }
  }

  // 3) Chờ hết chu kỳ
  uint32_t elapsed = millis() - t0;
  if (elapsed < PERIOD_MS) delay(PERIOD_MS - elapsed);
}
*/
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>

// ===== LoRa pins (ESP32 DOIT + RA-01/RA-02) =====
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    27     // <-- đúng theo bạn đang cắm
#define LORA_RST   14
#define LORA_DIO0  26
#define LORA_FREQ  433E6

SPIClass spiLoRa(VSPI);

// ===== RELAY params =====
static const int RELAY_ID = 10;   // tùy bạn đặt (10, 11,...)

// Forward lên Gateway: TYPE=FWD;REL=10; + payload gốc
static void forwardToGateway(const String& nodePayload) {
  String fwd;
  fwd.reserve(nodePayload.length() + 24);
  fwd = "TYPE=FWD;REL=";
  fwd += String(RELAY_ID);
  fwd += ";";
  fwd += nodePayload;

  // TX xong quay lại RX (rất quan trọng)
  LoRa.idle();
  LoRa.beginPacket();
  LoRa.print(fwd);
  LoRa.endPacket();      // blocking send
  LoRa.receive();        // quay lại RX ngay

  Serial.print("FWD->GW: ");
  Serial.println(fwd);
}

static void loraEnterRx() {
  LoRa.receive(); // RX continuous
}

void setup() {
  Serial.begin(9600);
  delay(300);

  // SPI + LoRa init
  spiLoRa.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(spiLoRa);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQ)) {
    Serial.println("LoRa init failed! Check wiring/pins/freq.");
    while (1) delay(1000);
  }

  // Phải giống Node/Gateway
  LoRa.setSyncWord(0x12);
  LoRa.enableCrc();
  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);

  loraEnterRx();
  Serial.println("RELAY ready: LoRa RX (from NODE) -> FWD (to GW)");
  Serial.print("Pins: SCK="); Serial.print(LORA_SCK);
  Serial.print(" MISO="); Serial.print(LORA_MISO);
  Serial.print(" MOSI="); Serial.print(LORA_MOSI);
  Serial.print(" SS="); Serial.print(LORA_SS);
  Serial.print(" RST="); Serial.print(LORA_RST);
  Serial.print(" DIO0="); Serial.println(LORA_DIO0);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

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

  // ===== Chỉ forward gói từ NODE =====
  // Node STM32 của bạn: luôn có "ID=" và KHÔNG có "TYPE="
  const bool isNodePacket = (payload.indexOf("ID=") >= 0) && (payload.indexOf("TYPE=") < 0);

  if (isNodePacket) {
    forwardToGateway(payload);
  } else {
    // Không forward: gói control (TYPE=...) hoặc rác
    // vẫn đảm bảo về RX
    loraEnterRx();
  }
}
