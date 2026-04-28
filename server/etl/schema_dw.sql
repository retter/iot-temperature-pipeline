-- =========================
-- DIMENSIONS
-- =========================

CREATE TABLE IF NOT EXISTS dim_device (
    device_id TEXT PRIMARY KEY
);

CREATE TABLE IF NOT EXISTS dim_date (
    date_id DATE PRIMARY KEY,
    year INT,
    month INT,
    day INT
);

CREATE TABLE IF NOT EXISTS dim_time (
    time_id TIME PRIMARY KEY,
    hour INT,
    minute INT,
    second INT
);

-- =========================
-- FACT TABLE
-- =========================

CREATE TABLE IF NOT EXISTS fact_temperature (
    id SERIAL PRIMARY KEY,

    -- foreign keys
    device_id TEXT REFERENCES dim_device(device_id),
    date_id DATE REFERENCES dim_date(date_id),
    time_id TIME REFERENCES dim_time(time_id),

    temperature NUMERIC,

    -- critical for ETL correctness
    source_id INT UNIQUE
);