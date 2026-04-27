#include "LoRaWan_APP.h"
#include "Arduino.h"
#include <WiFi.h>
#include <HTTPClient.h>

/* ================= WIFI ================= */
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

/* ================= HTTP INGEST ================= */
const char* serverUrl = "http://<SERVER_IP>:5000/sensor";

/* ================= LORA CONFIG ================= */
#define RF_FREQUENCY 915000000
#define LORA_BANDWIDTH 0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

static RadioEvents_t RadioEvents;

/* ================= SHARED BUFFER (ISR SAFE) ================= */
volatile bool packetReady = false;

uint8_t rxBuffer[64];
uint16_t rxSize = 0;

/* ================= WIFI CONNECT ================= */
void connectWiFi()
{
    WiFi.begin(ssid, password);

    Serial.print("WiFi connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi connected");
}

/* ================= LORA CALLBACK (DO NOT DO WORK HERE) ================= */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
    // ONLY COPY DATA — NO NETWORKING HERE
    if (size > sizeof(rxBuffer)) return;

    memcpy(rxBuffer, payload, size);
    rxSize = size;
    packetReady = true;
}

/* ================= PROCESS PACKET (SAFE CONTEXT) ================= */
void processPacket()
{
    if (!packetReady) return;

    packetReady = false;

    if (rxSize < 6) {
        Serial.println("Invalid packet size");
        Radio.Rx(0);
        return;
    }

    // decode temperature
    int16_t temp_int = (rxBuffer[4] << 8) | rxBuffer[5];
    float temperature = temp_int / 100.0;

    /* ================= EXTRA SAFETY FILTER ================= */
    if (temperature < -40 || temperature > 85) {
        Serial.print("Discarding invalid temp: ");
        Serial.println(temperature);
        Radio.Rx(0);
        return;
    }

    /* ================= BUILD JSON ================= */
    char json[128];
    snprintf(json, sizeof(json),
        "{\"device_id\":\"heltec-1\",\"temperature\":%.2f}",
        temperature
    );

    Serial.print("Sending HTTP: ");
    Serial.println(json);

    /* ================= HTTP SEND ================= */
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(serverUrl);
        http.addHeader("Content-Type", "application/json");

        int code = http.POST(json);

        Serial.print("HTTP response: ");
        Serial.println(code);

        http.end();
    }
    else
    {
        Serial.println("WiFi lost");
        connectWiFi();
    }

    /* ================= RESTART RX ================= */
    Radio.Rx(0);
}

/* ================= SETUP ================= */
void setup()
{
    Serial.begin(115200);

    connectWiFi();

    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    RadioEvents.RxDone = OnRxDone;
    Radio.Init(&RadioEvents);

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetRxConfig(MODEM_LORA,
        LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR,
        LORA_CODINGRATE,
        0,
        LORA_PREAMBLE_LENGTH,
        LORA_SYMBOL_TIMEOUT,
        LORA_FIX_LENGTH_PAYLOAD_ON,
        0,
        true,
        0,
        0,
        LORA_IQ_INVERSION_ON,
        true
    );

    Serial.println("LoRa Gateway Ready");

    Radio.Rx(0);
}

/* ================= LOOP ================= */
void loop()
{
    Radio.IrqProcess();   // REQUIRED for LoRa stack stability
    processPacket();      // safe processing outside ISR
}