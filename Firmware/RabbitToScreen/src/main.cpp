#include <WiFi.h>
#include <PubSubClient.h>
#include <TFT_eSPI.h>

/* ---------------- Wi-Fi Ayarları ---------------- */
const char* ssid     = "Mert";
const char* password = "12345678";

/* ------------ MQTT Kimlik Bilgileri ------------ */
const char* mqttUser = "/:mert";
const char* mqttPass = "1234";

IPAddress mqttBrokerIp;

WiFiClient    espClient;
PubSubClient  mqttClient(espClient);
TFT_eSPI      tft = TFT_eSPI();

/* --------------------------------------------------
 *  Türkçe → Latin ASCII çevirici
 * -------------------------------------------------- */
String latinify(const String& s) {
  String t = s;
  t.replace("ç","c"); t.replace("Ç","C");
  t.replace("ı","i"); t.replace("İ","I");
  t.replace("ö","o"); t.replace("Ö","O");
  t.replace("ğ","g"); t.replace("Ğ","G");
  t.replace("ş","s"); t.replace("Ş","S");
  t.replace("ü","u"); t.replace("Ü","U");
  return t;
}

/* --------------------------------------------------
 *  Durum mesajını üst çubuğa
 * -------------------------------------------------- */
void displayMessage(const String& msg) {
  tft.fillRect(0, 0, tft.width(), 30, TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(2);
  tft.drawString(msg, tft.width() / 2, 15);
}

/* --------------------------------------------------
 *  Ürün bilgisini ekranın ortasına hizalayarak göster
 * -------------------------------------------------- */
void displayProductInfo(const String& msg) {
  String parts[4];
  int start = 0;
  for (int i = 0; i < 3; ++i) {
    int idx = msg.indexOf(':', start);
    if (idx < 0) idx = msg.length();
    String seg = msg.substring(start, idx);
    seg.trim();
    parts[i] = latinify(seg);
    start = idx + 1;
  }
  String seg = msg.substring(start);
  seg.trim();
  parts[3] = latinify(seg);

  // Ekranı beyaz yap
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  tft.setTextDatum(TC_DATUM);  // üstten ortala

  int screenW = tft.width();
  int screenH = tft.height();
  int lineHeight = 25;
  int totalHeight = lineHeight * 4;
  int startY = (screenH - totalHeight) / 2 + lineHeight/2;

  // 4 satırı ekranın ortasına hizalayarak yaz
  tft.drawString("Urun: "    + parts[0], screenW/2, startY + 0*lineHeight);
  tft.drawString("Beden: "   + parts[1], screenW/2, startY + 1*lineHeight);
  tft.drawString("Renk: "    + parts[2], screenW/2, startY + 2*lineHeight);
  tft.drawString("Stok adet: "+ parts[3], screenW/2, startY + 3*lineHeight);
}

/* ---------------- MQTT Callback ------------------ */
void callback(char* topic, byte* payload, unsigned int length) {
  String payloadTxt;
  for (unsigned int i = 0; i < length; i++) {
    payloadTxt += (char)payload[i];
  }
  displayProductInfo(payloadTxt);
  Serial.printf(">> [%s] %s\n", topic, payloadTxt.c_str());
}

/* -------------- MQTT Reconnect ------------------- */
void reconnect() {
  while (!mqttClient.connected()) {
    String clientId = "ESP32-" + String(random(0xffff), HEX);
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPass)) {
      displayMessage("MQTT: OK");
      // Subscribe to this cabin's specific channel:
      const int cabinId = 2;
      String topic = String("target_queue.cabin") + cabinId;
      mqttClient.subscribe(topic.c_str(), 1);
      Serial.print("Subscribed to ");
      Serial.println(topic);
    } else {
      displayMessage("MQTT Err:" + String(mqttClient.state()));
      delay(2000);
    }
  }
}

/* -------------------- SETUP ---------------------- */
void setup() {
  Serial.begin(9600);

  // TFT başlangıç
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  displayMessage("Waiting Wi-Fi");

  // Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  displayMessage("Wi-Fi: OK");

  // MQTT Broker
  mqttBrokerIp = WiFi.gatewayIP();
  mqttClient.setServer(mqttBrokerIp, 1883);
  mqttClient.setCallback(callback);
  mqttClient.setBufferSize(512);
}

/* -------------------- LOOP ----------------------- */
void loop() {
  if (!mqttClient.connected()) reconnect();
  mqttClient.loop();
}
