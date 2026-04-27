from flask import Flask, request
import json
import time
from kafka import KafkaProducer

app = Flask(__name__)

def create_producer():
    for i in range(30):
        try:
            print(f"Connecting to Kafka... attempt {i}")
            producer = KafkaProducer(
                bootstrap_servers='kafka:9092',
                value_serializer=lambda v: json.dumps(v).encode('utf-8')
            )
            print("Kafka connected")
            return producer
        except Exception as e:
            print("Kafka not ready:", e)
            time.sleep(2)

    raise Exception("Kafka unavailable after retries")

producer = create_producer()

@app.route("/sensor", methods=["POST"])
def sensor():
    if not request.is_json:
    return {"error": "Invalid JSON"}, 400

    data = request.json
    print("Received:", data)

    producer.send("sensor.raw", data)

    return {"status": "ok"}

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
