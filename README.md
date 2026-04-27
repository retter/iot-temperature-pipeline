# IoT Temperature Pipeline (Heltec V3 → LoRa → WiFi → Kafka → PostgreSQL)

## Overview

This project implements an end-to-end IoT data pipeline:

* **Heltec V3 transmitter** reads internal temperature and sends via LoRa
* **Heltec V3 receiver/gateway** forwards data via WiFi (HTTP POST)
* **Flask ingest service** receives data and publishes to Kafka
* **Kafka** streams the data
* **Postgres writer** consumes Kafka messages and stores them
* **PostgreSQL** stores temperature data

---

## Architecture

```text
┌──────────────────────────┐
│ Heltec V3 Transmitter    │
│ - reads temperature      │
│ - LoRa TX                │
└─────────────┬────────────┘
              │ LoRa
              ▼
┌──────────────────────────┐
│ Heltec V3 Receiver       │
│ - LoRa RX                │
│ - WiFi connection        │
│ - HTTP POST              │
└─────────────┬────────────┘
              │ HTTP
              ▼
┌──────────────────────────┐
│ Flask Ingest API         │
│ POST /sensor             │
└─────────────┬────────────┘
              │ Kafka publish
              ▼
┌──────────────────────────┐
│ Kafka (sensor.raw topic) │
└─────────────┬────────────┘
              │ consume
              ▼
┌──────────────────────────┐
│ Postgres Writer          │
│ - Kafka consumer         │
│ - Inserts into DB        │
└─────────────┬────────────┘
              ▼
┌──────────────────────────┐
│ PostgreSQL               │
│ sensor_raw table         │
└──────────────────────────┘
```

---

## Repository Structure

```text
.
├── firmware/
│   ├── transmitter-heltec-v3/
│   │   └── transmitter.ino
│   └── receiver-heltec-v3/
│       └── receiver.ino
│
├── server/
│   ├── kafka-http-ingest/
│   │   ├── app.py
│   │   └── Dockerfile
│   │
│   └── postgres-writer/
│       ├── app.py
│       ├── Dockerfile
│       └── requirements.txt
│
├── compose.yaml
└── README.md
```

---

## Firmware

Developed using Arduino IDE

### Transmitter

* Reads internal chip temperature
* Sends LoRa packet every ~15 seconds

### Receiver / Gateway

* Receives LoRa packet
* Connects to WiFi
* Sends JSON via HTTP POST

Example payload:

```json
{
  "device_id": "heltec-1",
  "temperature": 42.53
}
```

---

## Backend (Dockerized)

Uses Docker and Docker Compose

### Services

* **kafka-http-ingest**

  * Flask API (`POST /sensor`)
  * Publishes to Kafka topic `sensor.raw`

* **kafka**

  * Message broker

* **postgres-writer**

  * Kafka consumer
  * Writes to PostgreSQL

* **postgres**

  * Database

---

## Quick Start

### 1. Clone repo

```bash
git clone https://github.com/retter/iot-temperature-pipeline.git
cd iot-temperature-pipeline
```

---

### 2. Start backend

```bash
docker compose up -d --build
```

---

### 3. Configure receiver

Edit:

```cpp
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* serverUrl = "http://<SERVER_IP>:5000/sensor";
```

```text
# Networking note:
# - If your server is on the same LAN → use a local IP (e.g., 192.168.x.x)
# - If your server is remote → you’ll need port forwarding or a public IP
```

---

### 4. Flash devices

Use Arduino IDE:

* Upload transmitter to one Heltec V3
* Upload receiver to another

---

### 5. Verify system

```bash
docker compose logs -f
```

You should see:

* Incoming HTTP requests
* Kafka messages
* Database inserts

---

## Database

Table auto-created:

```sql
sensor_raw (
  id SERIAL PRIMARY KEY,
  device_id TEXT,
  temperature NUMERIC,
  received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
)
```

Query:

```bash
docker exec -it postgres psql -U iotuser -d iot
```

```sql
SELECT * FROM sensor_raw ORDER BY received_at DESC;
```

---

## License

MIT - see LICENSE file
