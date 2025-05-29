#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 22  // Reset pin for MFRC522
#define SS_PIN  5   // SPI chip select

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;  // Authentication key

void setup() {
    Serial.begin(9600);
    while (!Serial);
    SPI.begin();
    mfrc522.PCD_Init();

    // Default authentication key (factory default: 0xFFFFFFFFFFFF)
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }
    delay(4);
    mfrc522.PCD_DumpVersionToSerial();
    Serial.println(F("Scan an RFID card to write unique key data to Block 4..."));
}

void loop() {
    // Yeni kart var mı kontrol et
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    byte block = 4;  // 4. blok kullanıcı verisi için kullanılacak

    // Örnek unique key: "MERT2025ABCD1234" (16 bayt)
    byte newData[16] = {
        'M','E','R','T','2','0','2','5',
        'G','M','L','K','9','9','9','9'
    };

    // Key A ile doğrulama (default key: 0xFFFFFFFFFFFF)
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, block, &key, &(mfrc522.uid));
    
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // 4. bloğa 16 bayt veri yaz
    status = mfrc522.MIFARE_Write(block, newData, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Write failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    } else {
        Serial.println(F("Write successful!"));
    }

    // Yazılan veriyi doğrulamak için 4. bloğu oku
    byte buffer[18];
    byte bufferSize = sizeof(buffer);
    status = mfrc522.MIFARE_Read(block, buffer, &bufferSize);
    
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Read failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
    } else {
        Serial.print(F("Data in Block "));
        Serial.print(block);
        Serial.print(F(": "));
        // ASCII olarak yazdır (örneğin: MERT2025ABCD1234)
        for (byte i = 0; i < 16; i++) {
            Serial.print((char)buffer[i]);
        }
        Serial.println();
    }

    // Kartı durdur ve şifrelemeyi kapat
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    
    delay(2000);  // Çok sık yazmayı engellemek için kısa bekleme
}
