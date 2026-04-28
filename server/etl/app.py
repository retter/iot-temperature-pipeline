import time
import psycopg2

# =========================
# CONNECT TO POSTGRES
# =========================
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

# =========================
# CREATE DW TABLES
# =========================
with open("/app/schema_dw.sql", "r") as f:
    cur.execute(f.read())
    conn.commit()

print("DW tables ready")

# =========================
# ETL LOOP
# =========================
while True:
    print("Running ETL...")

    cur.execute("""
        SELECT id, device_id, temperature, received_at
        FROM sensor_raw
    """)

    rows = cur.fetchall()

    for row_id, device_id, temperature, ts in rows:
        date_id = ts.date()
        time_id = ts.time().replace(microsecond=0)

        # -------------------------
        # DIMENSIONS
        # -------------------------

        cur.execute("""
            INSERT INTO dim_device (device_id)
            VALUES (%s)
            ON CONFLICT DO NOTHING
        """, (device_id,))

        cur.execute("""
            INSERT INTO dim_date (date_id, year, month, day)
            VALUES (%s, %s, %s, %s)
            ON CONFLICT DO NOTHING
        """, (date_id, date_id.year, date_id.month, date_id.day))

        cur.execute("""
            INSERT INTO dim_time (time_id, hour, minute, second)
            VALUES (%s, %s, %s, %s)
            ON CONFLICT DO NOTHING
        """, (time_id, time_id.hour, time_id.minute, time_id.second))

        # -------------------------
        # FACT (idempotent insert)
        # -------------------------

        cur.execute("""
            INSERT INTO fact_temperature (
                device_id, date_id, time_id, temperature, source_id
            )
            VALUES (%s, %s, %s, %s, %s)
            ON CONFLICT (source_id) DO NOTHING
        """, (device_id, date_id, time_id, temperature, row_id))

    conn.commit()

    time.sleep(10)