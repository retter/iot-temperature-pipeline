import json
import time
import psycopg2
from kafka import KafkaConsumer

while True:
    try:
        conn = psycopg2.connect(
            host="postgres",
            dbname="iot",
            user="iotuser",
            password="iotpass"
        )
        break
    except Exception as e:
        print("Waiting for Postgres:", e)
        time.sleep(5)

cur = conn.cursor()

cur.execute("""
CREATE TABLE IF NOT EXISTS sensor_raw (
    id SERIAL PRIMARY KEY,
    device_id TEXT,
    temperature NUMERIC,
    received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
)
""")
conn.commit()

consumer = KafkaConsumer(
    'sensor.raw',
    bootstrap_servers='kafka:9092',
    auto_offset_reset='earliest',
    group_id='postgres-writer',
    value_deserializer=lambda m: json.loads(m.decode('utf-8'))
)

for msg in consumer:
    data = msg.value

    print("Kafka payload:", data)

    cur.execute(
        """
        INSERT INTO sensor_raw (device_id, temperature)
        VALUES (%s, %s)
        """,
        (
            data.get("device_id"),
            data.get("temperature")
        )
    )

    conn.commit()
