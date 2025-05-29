#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// Fonksiyon bildirimi
void connectToMQTT();

// -- MFRC522 Ayarları --
#define RST_PIN 22    // Reset pin
#define SS_PIN  5     // SPI chip select
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  // Authentication key

// -- Wi-Fi Ayarları --
const char* ssid     = "Mert";        // Wi-Fi ağ adı
const char* password = "12345678";    // Wi-Fi şifresi

// -- RabbitMQ (MQTT) Ayarları --
const int   mqttPort     = 1883;      // MQTT portu
const char* mqttUser     = "mert";    // RabbitMQ kullanıcı adı
const char* mqttPassword = "1234";    // RabbitMQ şifresi

// -- Kabin ID (1 veya 2 olarak değiştirin) --
const int cabinId = 2;

// MQTT istemcisi için global nesneler
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Zamanlayıcılar
unsigned long lastHeartbeatTime    = 0;
const unsigned long heartbeatInterval    = 600000;  // 10 dakika
unsigned long lastDataPublishTime = 0;
const unsigned long dataPublishCooldown   = 5000;    // 5 saniye

String lastData = "";  // En son yayınlanan veri

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // MFRC522 başlat
  SPI.begin();
  mfrc522.PCD_Init();
  delay(4);
  mfrc522.PCD_DumpVersionToSerial();
  Serial.println(F("Scan an RFID card to read data from Block 4..."));

  // Varsayılan authentication key
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  // Wi-Fi bağlantısı
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway IP: ");
  Serial.println(WiFi.gatewayIP());

  // MQTT ayarları
  mqttClient.setKeepAlive(30);
  mqttClient.setServer(WiFi.gatewayIP(), mqttPort);

  // MQTT bağlantısı
  connectToMQTT();

  lastHeartbeatTime = millis();
}

void loop() {
  if (!mqttClient.connected()) {
    connectToMQTT();
  }
  mqttClient.loop();

  unsigned long currentMillis = millis();

  // 10 dakikada bir heartbeat
  if (currentMillis - lastHeartbeatTime >= heartbeatInterval) {
    mqttClient.publish("system/heartbeat", "System OK");
    Serial.println("Heartbeat sent: System OK");
    lastHeartbeatTime = currentMillis;
  }

  // RFID kart okuma
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  byte block = 4;
  byte buffer[18];
  byte bufferSize = sizeof(buffer);

  // Authentication
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Auth failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  // Read block 4
  status = mfrc522.MIFARE_Read(block, buffer, &bufferSize);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("Read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  } else {
    // 16 baytı string'e çevir
    String dataString;
    for (byte i = 0; i < 16; i++) {
      dataString += (char)buffer[i];
    }

    // Yeni payload: "ID Cabin X"
    String payload = dataString + " Cabin " + String(cabinId);

    // Soğuma ve tekrar yayını kontrolü
    if (payload != lastData || (currentMillis - lastDataPublishTime >= dataPublishCooldown)) {
      mqttClient.publish("test_queue", payload.c_str());
      Serial.print("Published Payload: ");
      Serial.println(payload);
      lastData = payload;
      lastDataPublishTime = currentMillis;
    }
  }

  // Kartı durdur ve şifrelemeyi kapat
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void connectToMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Connecting to RabbitMQ (MQTT)...");
    String clientId = "ESP32ClientID-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword)) {
      Serial.print("MQTT connected! Client ID: ");
      Serial.println(clientId);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retry in 2 seconds");
      delay(2000);
    }
  }
}
