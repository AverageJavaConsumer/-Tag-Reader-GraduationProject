#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sqlite3
import pika
import netifaces

# -------------------------------------------------------------------
# Dinamik IP (dokunulmadı)
# -------------------------------------------------------------------
def get_local_ip():
    print("[DEBUG] get_local_ip() fonksiyonu çalışıyor...")
    for iface in netifaces.interfaces():
        addrs = netifaces.ifaddresses(iface)
        if netifaces.AF_INET in addrs:
            for addr_info in addrs[netifaces.AF_INET]:
                ip = addr_info.get('addr')
                if ip and not ip.startswith("127.") and ip.startswith("192.168."):
                    print(f"[DEBUG] Tercih edilen IP bulundu: {ip} (Arayüz: {iface})")
                    return ip
    for iface in netifaces.interfaces():
        addrs = netifaces.ifaddresses(iface)
        if netifaces.AF_INET in addrs:
            for addr_info in addrs[netifaces.AF_INET]:
                ip = addr_info.get('addr')
                if ip and not ip.startswith("127."):
                    print(f"[DEBUG] Alternatif IP bulundu: {ip} (Arayüz: {iface})")
                    return ip
    print("[DEBUG] Uygun IP bulunamadı, varsayılan olarak 127.0.0.1 döndürülüyor.")
    return "127.0.0.1"

# -------------------------------------------------------------------
# Ayarlar
# -------------------------------------------------------------------
RABBITMQ_SERVER = get_local_ip()
RABBITMQ_PORT   = 5672
QUEUE_NAME      = "test_queue"      # RFID okuma kuyruğu (gelen)
TARGET_QUEUE    = "target_queue"    # ESP32'nin de tüketeceği ana kuyruk

DB_FILENAME     = "test.db"

# -------------------------------------------------------------------
# SQLite yardımcıları
# -------------------------------------------------------------------
def create_table():
    try:
        conn = sqlite3.connect(DB_FILENAME)
        cursor = conn.cursor()
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS clothes (
                id        INTEGER PRIMARY KEY AUTOINCREMENT,
                rfid_key  TEXT NOT NULL,
                name      TEXT,
                size      TEXT,
                color     TEXT,
                stok      INTEGER
            );
        """)
        conn.commit()
        print("[DEBUG] 'clothes' tablosu oluşturuldu veya mevcut.")
    except sqlite3.Error as e:
        print("[DEBUG] Tablo oluşturma hatası:", e)
    finally:
        conn.close()

def insert_item(rfid_key, name, size, color, stok):
    print("[DEBUG] insert_item() fonksiyonu çalışıyor...")
    try:
        conn = sqlite3.connect(DB_FILENAME)
        cursor = conn.cursor()
        cursor.execute("""
            INSERT INTO clothes (rfid_key, name, size, color, stok)
            VALUES (?, ?, ?, ?, ?)
        """, (rfid_key, name, size, color, stok))
        conn.commit()
        print(f"[DEBUG] Item '{rfid_key}' eklendi: {name}, {size}, {color}, stok: {stok}")
    except sqlite3.Error as e:
        print("[DEBUG] SQLite insert hatası:", e)
    finally:
        conn.close()
        print("[DEBUG] SQLite bağlantısı kapatıldı (insert_item).")

def query_database_return_one(unique_key):
    print("[DEBUG] query_database_return_one() fonksiyonu çalışıyor...")
    try:
        conn = sqlite3.connect(DB_FILENAME)
        cursor = conn.cursor()
        cursor.execute("""
            SELECT name, size, color, stok
            FROM clothes
            WHERE rfid_key = ?
            LIMIT 1
        """, (unique_key,))
        row = cursor.fetchone()
        if row:
            print("[DEBUG] Bulunan kayıt:", row)
            return row  # (name, size, color, stok)
        else:
            print("[DEBUG] Sorgu sonucu bulunamadı.")
            return (None, None, None, None)
    except sqlite3.Error as e:
        print("[DEBUG] SQLite sorgu hatası:", e)
        return (None, None, None, None)
    finally:
        conn.close()
        print("[DEBUG] SQLite bağlantısı kapatıldı (query_database_return_one).")

# -------------------------------------------------------------------
# RabbitMQ callback
# -------------------------------------------------------------------
def callback(ch, method, properties, body):
    print("[DEBUG] callback() fonksiyonu çalıştı, mesaj alındı.")
    raw = body.decode("utf-8")
    print(f"[RabbitMQ] Mesaj alındı: {raw}")

    # Expecting format "MERT…ID… Cabin X"
    if " Cabin " in raw:
        rfid_key, cabin_part = raw.split(" Cabin ", 1)
        cabin_id = cabin_part.strip()
    else:
        rfid_key = raw
        cabin_id = "1"  # default cabin

    name, size, color, stok = query_database_return_one(rfid_key)
    new_payload = f"{name}:{size}:{color}:{stok}" if name else "No Data"

    # Publish only to the specific cabin’s topic
    routing_key = f"{TARGET_QUEUE}.cabin{cabin_id}"
    ch.basic_publish(
        exchange="amq.topic",
        routing_key=routing_key,
        body=new_payload.encode()          ### only change ①
    )
    print(f"[DEBUG] Mesaj gönderildi: queue='{routing_key}', body='{new_payload}'")

    ch.basic_ack(delivery_tag=method.delivery_tag)
    print("[DEBUG] Mesaj onaylandı.")


# -------------------------------------------------------------------
# main
# -------------------------------------------------------------------
def main():
    print("[DEBUG] main() fonksiyonu başladı.")

    create_table()
    # örnek veriler
    insert_item("MERT2025EFGH5678", "T-Shirt",     "M", "Yeşil",      5)
    insert_item("MERT2025ABCD1234", "Kot Pantolon","L", "Koyu Mavi",  3)
    insert_item("MERT2025ETEK0001", "Etek",        "S", "Siyah",     10)
    insert_item("MERT2025GMLK9999", "Gömlek",      "XL","Beyaz",     24)
    print(f"[DEBUG] RabbitMQ: {RABBITMQ_SERVER}:{RABBITMQ_PORT}")
    params = pika.ConnectionParameters(host=RABBITMQ_SERVER, port=RABBITMQ_PORT)

    try:
        connection = pika.BlockingConnection(params)
        print("[DEBUG] RabbitMQ bağlantısı başarılı!")
    except Exception as e:
        print("[DEBUG] RabbitMQ bağlantı hatası:", e)
        return

    channel = connection.channel()
    print("[DEBUG] Kanal oluşturuldu.")

    # Kuyruklar
    channel.queue_declare(queue=TARGET_QUEUE, durable=True)
    channel.queue_declare(queue=QUEUE_NAME,  durable=True)

    # amq.topic → target_queue binding (guaranteed)   ### only change ②
    channel.exchange_declare(exchange="amq.topic", exchange_type="topic", durable=True)
    channel.queue_bind(exchange="amq.topic",
                       queue=TARGET_QUEUE,
                       routing_key=f"{TARGET_QUEUE}.cabin*")

    # test_queue dinleniyor
    channel.basic_consume(queue=QUEUE_NAME, on_message_callback=callback)
    print("[DEBUG] Abonelik başarılı. Mesajlar bekleniyor...\n")

    try:
        channel.start_consuming()
    except KeyboardInterrupt:
        print("\n[DEBUG] Dinleme KeyboardInterrupt ile durduruldu.")
        channel.stop_consuming()
    finally:
        connection.close()
        print("[DEBUG] RabbitMQ bağlantısı kapatıldı.")

if __name__ == "__main__":
    main()
