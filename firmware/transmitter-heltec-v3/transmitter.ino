#include "driver/temperature_sensor.h"
#include "LoRaWan_APP.h"
#include "Arduino.h"

/* temperature sensor */
temperature_sensor_handle_t temp_handle = NULL;

/* LoRa radio params (must match receiver) */
#define RF_FREQUENCY 915000000 // Hz
#define TX_OUTPUT_POWER 14     // dBm
#define LORA_BANDWIDTH 0       // 125 kHz
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 1      // 4/5
#define LORA_PREAMBLE_LENGTH 8
#define LORA_SYMBOL_TIMEOUT 0
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

uint32_t appTxDutyCycle = 15000;


/* Radio events */
static RadioEvents_t RadioEvents;

static void prepareTxFrame()
{
    float temperatureC = 0;
    temperature_sensor_get_celsius(temp_handle, &temperatureC);

    int16_t temp_int = (int16_t)(temperatureC * 100);

    appDataSize = 6;
    appData[0] = 0x00;
    appData[1] = 0x01;
    appData[2] = 0x02;
    appData[3] = 0x03;
    appData[4] = (temp_int >> 8) & 0xFF;
    appData[5] = temp_int & 0xFF;

    Serial.print("Temp: ");
    Serial.println(temperatureC);
}

void setup()
{
    Serial.begin(115200);

    /* temp sensor */
    temperature_sensor_config_t temp_config = {
        .range_min = 20,
        .range_max = 50,
    };
    temperature_sensor_install(&temp_config, &temp_handle);
    temperature_sensor_enable(temp_handle);

    /* init radio */
    Mcu.begin(HELTEC_BOARD, SLOW_CLK_TPYE);

    RadioEvents.TxDone = NULL;
    Radio.Init(&RadioEvents);

    Radio.SetChannel(RF_FREQUENCY);

    Radio.SetTxConfig(MODEM_LORA, TX_OUTPUT_POWER, 0,
                     LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                     LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
                     LORA_FIX_LENGTH_PAYLOAD_ON, true, 0, 0,
                     LORA_IQ_INVERSION_ON, 3000);

    Serial.println("LoRa RAW Sender Ready");
}

void loop()
{
    prepareTxFrame();

    Radio.Send(appData, appDataSize);

    Serial.println("Packet sent");

    delay(appTxDutyCycle);
}