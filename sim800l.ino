#include <Arduino.h>

HardwareSerial gsm(2); // RX2=GPIO16, TX2=GPIO17

// ====== EDITADO PARA EQUIPO 4 Y TU NÚMERO ======
const char* PHONE_NUMBER = "+523131438961";
const char* APN       = "TU_APN";     // <-- APN real
const char* APN_USER  = "";           // <-- si el operador lo pide
const char* APN_PASS  = "";           // <-- si el operador lo pide

const char* MQTT_HOST = "test.mosquitto.org";
const int   MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "equipo4-sim800l-esp32";
const char* MQTT_TOPIC_SUB = "equipo4/led"; // payload "ON"/"OFF"
// ================================================

const int LED_PIN = 2;  // LED interno ESP32
const uint32_t GSM_BAUD = 9600;

static String rx;

// ---- Utilidades AT ----
bool atWait(const String& expect, uint32_t timeout=5000) {
  uint32_t t0 = millis();
  rx = "";
  while (millis() - t0 < timeout) {
    while (gsm.available()) {
      char c = (char)gsm.read();
      rx += c;
      if (rx.indexOf(expect) >= 0) return true;
    }
    delay(1);
  }
  return false;
}

bool atCmd(const String& cmd, const String& expect="OK", uint32_t timeout=5000, bool echo=true) {
  if (echo) Serial.println(">> " + cmd);
  gsm.println(cmd);
  bool ok = atWait(expect, timeout);
  if (echo) Serial.println(rx);
  return ok;
}

bool sim800_init() {
  if (!atCmd("AT", "OK", 2000)) return false;
  atCmd("ATE0");                 // sin eco
  atCmd("AT+CMGF=1");            // SMS modo texto
  atCmd("AT+CLIP=1");            // caller ID
  // Esperar registro a red
  unsigned long t0 = millis();
  while (millis() - t0 < 30000) {
    gsm.println("AT+CREG?");
    if (atWait("+CREG:")) {
      if (rx.indexOf(",1")>=0 || rx.indexOf(",5")>=0) {
        Serial.println("[NET] Registrado en la red.");
        return true;
      }
    }
    delay(1000);
  }
  return false;
}

// ---- SMS ----
bool sendSMS(const char* number, const char* text) {
  if (!atCmd("AT+CMGF=1")) return false;
  gsm.print("AT+CMGS=\""); gsm.print(number); gsm.println("\"");
  if (!atWait(">")) return false;
  gsm.print(text);
  gsm.write(26); // CTRL+Z
  bool ok = atWait("OK", 15000);
  Serial.println(ok ? "[SMS] Enviado" : "[SMS] Falló");
  return ok;
}

// ---- Llamada ----
bool placeCallAndHangup(const char* number, uint16_t seconds=8) {
  String cmd = "ATD"; cmd += number; cmd += ";";
  if (!atCmd(cmd.c_str(), "OK", 5000)) return false;
  Serial.println("[CALL] Marcando...");
  delay(seconds * 1000);
  bool ok = atCmd("ATH");
  Serial.println(ok ? "[CALL] Colgado" : "[CALL] Falló colgar");
  return ok;
}

// ---- GPRS + TCP ----
bool gprs_up() {
  atCmd("AT+CIPSHUT", "SHUT OK", 10000);
  if (!atCmd("AT+CGATT=1")) return false;
  String cstt = "AT+CSTT=\""; cstt += APN; cstt += "\",\""; cstt += APN_USER; cstt += "\",\""; cstt += APN_PASS; cstt += "\"";
  if (!atCmd(cstt)) return false;
  if (!atCmd("AT+CIICR", "OK", 85000)) return false; // activa enlace
  if (!atCmd("AT+CIFSR")) return false;              // muestra IP
  return true;
}

bool tcp_connect(const char* host, int port) {
  String cmd = "AT+CIPSTART=\"TCP\",\""; cmd += host; cmd += "\",\""; cmd += String(port); cmd += "\"";
  return atCmd(cmd, "CONNECT OK", 30000);
}

bool tcp_send(const uint8_t* data, size_t len) {
  String cmd = "AT+CIPSEND=" + String(len);
  if (!atCmd(cmd, ">", 5000)) return false;
  gsm.write(data, len);
  return atWait("SEND OK", 10000);
}

int tcp_read(uint8_t* buf, size_t maxlen, uint32_t timeout=2000) {
  uint32_t t0 = millis();
  String tmp = "";
  while (millis() - t0 < timeout) {
    while (gsm.available()) {
      char c = (char)gsm.read();
      tmp += c;
      int p = tmp.indexOf("+IPD,");
      if (p >= 0) {
        int colon = tmp.indexOf(':', p);
        if (colon > p) {
          String slen = tmp.substring(p+5, colon);
          int n = slen.toInt();
          while ((int)tmp.length() - (colon+1) < n) {
            if (!gsm.available()) {
              if (millis() - t0 > timeout) break;
              delay(1); continue;
            }
            tmp += (char)gsm.read();
          }
          int got = min(n, (int)maxlen);
          memcpy(buf, tmp.c_str() + colon + 1, got);
          return got;
        }
      }
    }
    delay(1);
  }
  return 0;
}

// ---- MQTT mínimo (3.1.1) ----
uint16_t mqttPktLenEnc(uint8_t* out, uint32_t len) {
  uint16_t i=0;
  do { uint8_t enc = len % 128; len /= 128; if (len) enc |= 0x80; out[i++] = enc; } while (len && i<4);
  return i;
}
size_t mqttStr(uint8_t* p, const char* s) {
  uint16_t n = strlen(s); p[0] = (n>>8)&0xFF; p[1] = n&0xFF; memcpy(p+2, s, n); return n+2;
}

bool mqtt_connect() {
  uint8_t pkt[200]; uint8_t* p = pkt;
  *p++ = 0x10; // CONNECT
  uint8_t vh[180]; uint8_t* q = vh;
  q += mqttStr(q, "MQTT"); *q++ = 0x04; *q++ = 0x02; // clean session
  *q++ = 0x00; *q++ = 60;  // keepalive 60s
  q += mqttStr(q, MQTT_CLIENT_ID);
  uint8_t rem[4]; uint16_t remLen = mqttPktLenEnc(rem, q - vh);
  memcpy(p, rem, remLen); p += remLen; memcpy(p, vh, q - vh); p += (q - vh);
  size_t total = p - pkt;
  if (!tcp_send(pkt, total)) return false;

  uint8_t buf[32]; int n = tcp_read(buf, sizeof(buf), 8000);
  if (n >= 4 && buf[0] == 0x20 && buf[3] == 0x00) { Serial.println("[MQTT] CONNECT OK"); return true; }
  Serial.println("[MQTT] CONNECT FAIL"); return false;
}

bool mqtt_subscribe(const char* topic) {
  static uint16_t pid = 1;
  uint8_t pkt[256]; uint8_t* p = pkt; *p++ = 0x82; // SUBSCRIBE QoS1
  uint8_t vh[220]; uint8_t* q = vh; *q++ = (pid>>8)&0xFF; *q++ = pid&0xFF;
  q += mqttStr(q, topic); *q++ = 0x00; // QoS0
  uint8_t rem[4]; uint16_t remLen = mqttPktLenEnc(rem, q - vh);
  memcpy(p, rem, remLen); p += remLen; memcpy(p, vh, q - vh); p += (q - vh);
  size_t total = p - pkt;
  if (!tcp_send(pkt, total)) return false;
  uint8_t buf[32]; int n = tcp_read(buf, sizeof(buf), 8000);
  if (n >= 5 && buf[0] == 0x90 && buf[3] == ((pid>>8)&0xFF) && buf[4] == (pid&0xFF)) {
    Serial.println("[MQTT] SUBSCRIBED"); pid++; return true;
  }
  Serial.println("[MQTT] SUB FAIL"); return false;
}

void handle_mqtt_publish(const uint8_t* payload, int len) {
  String msg; msg.reserve(len);
  for (int i=0;i<len;i++) msg += (char)payload[i];
  msg.trim(); msg.toUpperCase();
  if (msg == "ON") { digitalWrite(LED_PIN, HIGH); Serial.println("[LED] ON"); }
  else if (msg == "OFF") { digitalWrite(LED_PIN, LOW); Serial.println("[LED] OFF"); }
  else { Serial.print("[MQTT] Payload desconocido: "); Serial.println(msg); }
}

void mqtt_loop() {
  uint8_t buf[512]; int n = tcp_read(buf, sizeof(buf), 500);
  if (n <= 0) return;
  int idx = 0;
  while (idx < n) {
    if (buf[idx] != 0x30 && buf[idx] != 0x32) { idx++; continue; } // PUBLISH QoS0/QoS1
    uint8_t hdr = buf[idx++];
    uint32_t rem = 0; uint8_t m=1, enc;
    do { if (idx>=n) return; enc = buf[idx++]; rem += (enc & 127) * m; m *= 128; } while (enc & 128);
    if (idx+1 >= n) return;
    uint16_t tlen = ((uint16_t)buf[idx]<<8) | buf[idx+1]; idx+=2;
    if (idx + tlen > n) return;
    idx += tlen; // omitimos el topic

    if ((hdr & 0x06) == 0x02) { if (idx+1 >= n) return; idx += 2; } // skip Packet Identifier (QoS1)

    int plen = rem - (2 + tlen) - (((hdr & 0x06)==0x02)?2:0);
    if (plen < 0 || idx + plen > n) return;
    handle_mqtt_publish(buf + idx, plen);
    idx += plen;
  }
}

void setup() {
  pinMode(LED_PIN, OUTPUT); digitalWrite(LED_PIN, LOW);
  Serial.begin(115200);
  gsm.begin(GSM_BAUD, SERIAL_8N1, 16, 17); // RX,TX
  Serial.println("\n[BOOT] ESP32 + SIM800L (Equipo 4)");

  if (!sim800_init()) { Serial.println("[ERR] SIM800L no listo o sin red."); return; }

  // (1) SMS a tu número
  sendSMS(PHONE_NUMBER, "Equipo 4: SMS OK desde SIM800L/ESP32");

  // (2) Llamada y colgado
  placeCallAndHangup(PHONE_NUMBER, 8);

  // (3) GPRS + MQTT
  Serial.println("[NET] Levantando GPRS...");
  if (!gprs_up()) { Serial.println("[ERR] GPRS no disponible."); return; }

  Serial.println("[TCP] Conectando a broker...");
  if (!tcp_connect(MQTT_HOST, MQTT_PORT)) { Serial.println("[ERR] TCP al broker falló."); return; }

  if (!mqtt_connect()) return;
  mqtt_subscribe(MQTT_TOPIC_SUB);
}

void loop() {
  mqtt_loop();
  static uint32_t t0 = millis();
  if (millis() - t0 > 25000) { t0 = millis(); uint8_t ping[2] = {0xC0, 0x00}; tcp_send(ping, 2); } // PINGREQ
}
