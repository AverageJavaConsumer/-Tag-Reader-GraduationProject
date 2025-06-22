Tag Reader – Smart Fitting Room System
======================================

🏆 3rd Place Winner – Genç Beyinler Yeni Fikirler 2025
📂 Category: Industry & Agriculture
🧑‍🎓 İzmir University of Economics – Graduation Project
👥 Mert Çakı, Onur Yağcı
🎓 Mentor: Prof. Dr. Yusuf Murat Erten

Project Description
--------------------
Tag Reader is an IoT-based smart fitting room system designed to improve the in-store customer experience. When a customer scans an RFID-tagged garment inside a fitting room, the system instantly displays product details such as name, size, color, and stock status on a dedicated TFT screen.

System Architecture
--------------------
RFID Scanner → ESP32 → RabbitMQ → Python Server → SQLite → ESP32 → TFT Display

Technologies Used
------------------
Layer        | Tools & Frameworks
-------------|-----------------------------
Hardware     | ESP32, MFRC522, TFT_eSPI
Firmware     | C++ / PlatformIO / PubSubClient
Backend      | Python 3, SQLite3, RabbitMQ, Pika
Messaging    | MQTT (PubSub) & AMQP (Exchange Routing)
Communication| 2.4GHz Wi-Fi

Folder Structure
-----------------
GraduationProject_TagReader/
├── Firmware/
│   ├── RabbitToScreen/
│   ├── RF-ID Writer/
│   └── TagReader_RabbitMQConnectionforEsp/
├── Backend/
│   └── rabbitmq_DataBaseQuerryApp/
├── Docs/
│   ├── poster.pdf
│   └── README.md

How to Run
-----------
Backend Server:
cd Backend/rabbitmq_DataBaseQuerryApp/
python main.py

ESP32 Firmware:
Upload using PlatformIO for each folder

RabbitMQ Server:
Ensure it's running. Follow the instructions on the link.

https://www.rabbitmq.com 


Key Features
-------------
- Modular (any screen/microcontroller)
- Supports multiple fitting rooms
- Lightweight MQTT communication
- Retail integration ready

Challenges Faced
-----------------
- PAHO MQTT failure in C++ → switched to Python
- ESP32 lacks AMQP → fixed with exchange + routing key
- Multi-cabin interference → fixed with wildcard subscriptions
