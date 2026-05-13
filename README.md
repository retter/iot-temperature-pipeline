# 📡 IoT Temperature Pipeline (LoRa → Kafka → Postgres → Data Warehouse)

This project is a complete end-to-end IoT data pipeline that captures temperature from a Heltec V3 board, transmits it over LoRa, ingests it via HTTP, streams through Kafka, stores raw data in PostgreSQL, and transforms it into a star-schema data warehouse for analytics.

---

# 🧭 Architecture Overview

```
[Heltec Sensor Node]
    ↓ (LoRa)
[Heltec Gateway Node]
    ↓ (WiFi HTTP POST)
[kafka-http-ingest (Flask)]
    ↓
[Kafka Topic: sensor.raw]
    ↓
[postgres-writer (consumer)]
    ↓
[PostgreSQL - sensor_raw]
    ↓
[ETL Container]
    ↓
[Star Schema Tables]
    ├── dim_device
    ├── dim_date
    ├── dim_time
    └── fact_temperature
```

---

# 🧱 Components

## 📟 IoT Layer

### Sensor Node (Heltec V3)

* Reads internal ESP32 temperature sensor
* Encodes temperature into LoRa payload
* Transmits at fixed interval

### Gateway Node (Heltec V3)

* Receives LoRa packets
* Decodes temperature
* Sends JSON via HTTP POST over WiFi

---

## 🌐 Ingestion Layer

### kafka-http-ingest (Flask)

* REST endpoint: `/sensor`
* Accepts JSON payload:

```json
{
  "device_id": "heltec-1",
  "temperature": 23.45
}
```

* Produces messages to Kafka topic:

```
sensor.raw
```

---

## 🔄 Streaming Layer

### Kafka

* Topic: `sensor.raw`
* Acts as buffer and decoupling layer
* Enables scalable consumers

---

## 🗄️ Storage Layer

### PostgreSQL (Raw Table)

```sql
sensor_raw (
    id SERIAL PRIMARY KEY,
    device_id TEXT,
    temperature NUMERIC,
    received_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
)
```

---

## 🧠 Data Warehouse Layer (Star Schema)

### Dimension Tables

```sql
dim_device (
    device_id TEXT PRIMARY KEY
)

dim_date (
    date_id DATE PRIMARY KEY,
    year INT,
    month INT,
    day INT
)

dim_time (
    time_id TIME PRIMARY KEY,
    hour INT,
    minute INT,
    second INT
)
```

---

### Fact Table

```sql
fact_temperature (
    id SERIAL PRIMARY KEY,
    device_id TEXT REFERENCES dim_device(device_id),
    date_id DATE REFERENCES dim_date(date_id),
    time_id TIME REFERENCES dim_time(time_id),
    temperature NUMERIC,
    source_id INT UNIQUE
)
```

### 🔑 Key Design Notes

* `source_id` links back to `sensor_raw.id`
* Ensures **idempotent ETL (no duplicates)**
* Enables safe reprocessing

---

## ⚙️ ETL Layer

### ETL Container

* Extracts data from `sensor_raw`
* Transforms:

  * Splits timestamp into date/time dimensions
  * Normalizes device IDs
* Loads into star schema

### Behavior

* Runs continuously
* Uses:

```sql
ON CONFLICT (source_id) DO NOTHING
```

➡️ Prevents duplicate inserts
➡️ Makes pipeline resilient

---

# 🐳 Docker Architecture

### Services

* `kafka`
* `postgres`
* `kafka-http-ingest`
* `postgres-writer`
* `etl`

---

### Run Everything

```bash
docker compose up -d --build
```

---

# 🌐 Network Notes

If your server is:

* same LAN → use local IP (e.g. 192.168.x.x)
* remote → you’ll need port forwarding or public IP

---

# 📊 Analytics (Power BI Ready)

The star schema enables:

* Time-series analysis
* Device comparisons
* Aggregations (avg/min/max)
* Heatmaps (time-of-day patterns)

---

# 📈 Examples

### Average temperature per day query

```sql
SELECT 
    d.date_id,
    AVG(f.temperature)
FROM fact_temperature f
JOIN dim_date d ON f.date_id = d.date_id
GROUP BY d.date_id
ORDER BY d.date_id;
```

---

### Average temperature per device query

```sql
SELECT 
    device_id,
    AVG(temperature)
FROM fact_temperature
GROUP BY device_id;
```

---

### Power BI Visualization

[Sum of Temperature by DateTime](/gallery/powerbivis_markedup.png)

---

# 📁 Project Structure

```
.
├── server/
│   ├── kafka-http-ingest/
│   ├── postgres-writer/
│   └── etl/
├── compose.yaml
├── README.md
```

---

# 📄 License (MIT) - see LICENSE file

