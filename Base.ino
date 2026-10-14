/**
 * @file LoRaP2P_RX.ino
 * @author rakwireless.com
 * @brief Receiver node for LoRa point to point communication
 * @version 0.1
 * @date 2020-08-21
 * 
 * @copyright Copyright (c) 2020
 * 
 * @note RAK4631 GPIO mapping to nRF52840 GPIO ports
   RAK4631    <->  nRF52840
   WB_IO1     <->  P0.17 (GPIO 17)
   WB_IO2     <->  P1.02 (GPIO 34)
   WB_IO3     <->  P0.21 (GPIO 21)
   WB_IO4     <->  P0.04 (GPIO 4)
   WB_IO5     <->  P0.09 (GPIO 9)
   WB_IO6     <->  P0.10 (GPIO 10)
   WB_SW1     <->  P0.01 (GPIO 1)
   WB_A0      <->  P0.04/AIN2 (AnalogIn A2)
   WB_A1      <->  P0.31/AIN7 (AnalogIn A7)
 */
#include <Arduino.h>
#include <SX126x-RAK4630.h> //http://librarymanager/All#SX126x
#include <SPI.h>

// Function declarations
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
void OnRxTimeout(void);
void OnRxError(void);

#ifdef NRF52_SERIES
#define LED_BUILTIN 35
#endif

// Define LoRa parameters
#define RF_FREQUENCY 915000000	// Hz
#define TX_OUTPUT_POWER 0		// dBm
#define LORA_BANDWIDTH 0		// [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1		// [1: 4/5, 2: 4/6,  3: 4/7,  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8	// Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 0	// Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define RX_TIMEOUT_VALUE 3000
#define TX_TIMEOUT_VALUE 3000

static RadioEvents_t RadioEvents;

static uint8_t RcvBuffer[64];

volatile bool restartRx = false;

//globals for sensor:
volatile bool newPacket = false;
volatile int16_t rxRssi = 0;
volatile int8_t rxSnr = 0;

void setup()
{

	// Initialize Serial for debug output
	time_t timeout = millis();
	Serial.begin(115200);
	while (!Serial)
	{
		if ((millis() - timeout) < 5000)
		{
            delay(100);
        }
        else
        {
            break;
        }
	}


	Serial.println("=====================================");
	Serial.println("LoRaP2P Rx Test");
	Serial.println("=====================================");
	// Initialize LoRa chip.
	lora_rak4630_init();
	// Initialize the Radio callbacks
	RadioEvents.TxDone = NULL;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = NULL;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;
	RadioEvents.CadDone = NULL;

	// Initialize the Radio
	Radio.Init(&RadioEvents);

	// Set Radio channel
	Radio.SetChannel(RF_FREQUENCY);

	// Set Radio RX configuration
	Radio.SetRxConfig(MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
					  LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
					  LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
					  0, true, 0, 0, LORA_IQ_INVERSION_ON, true);

	// Start LoRa
	Serial.println("Starting Radio.Rx");
	Radio.Rx(RX_TIMEOUT_VALUE);
}

void loop()
{
	if (newPacket) {
    newPacket = false;
    printReading((char *)RcvBuffer, rxRssi, rxSnr);
  }
  if (restartRx)
  {
    restartRx = false;
    Radio.Rx(0);
  }
  delay(10);
}

/**@brief Function to be executed on Radio Rx Done event
 */
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr)
{
  if (size > sizeof(RcvBuffer) - 1) size = sizeof(RcvBuffer) - 1;
  memcpy(RcvBuffer, payload, size);
  RcvBuffer[size] = '\0';   // end-of-text marker, so it prints as a string
  rxRssi = rssi;
  rxSnr = snr;
  newPacket = true;
  restartRx = true;
}

/**@brief Function to be executed on Radio Rx Timeout event
 */
void OnRxTimeout(void)
{
	Serial.println("OnRxTimeout");
	restartRx = true;
}

/**@brief Function to be executed on Radio Rx Error event
 */
void OnRxError(void)
{
	Serial.println("OnRxError");
	restartRx = true;
}

// Prints one received message with labels.
// Expects text like "S1,4,21.4,0.0" (node, msg #, temp, moisture).
void printReading(const char *text, int16_t rssi, int8_t snr)
{
  char buf[64];
  strncpy(buf, text, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  char *node  = strtok(buf, ",");
  char *count = strtok(NULL, ",");
  char *temp  = strtok(NULL, ",");
  char *moist = strtok(NULL, ",");

  Serial.print("Node: ");     Serial.print(node  ? node  : "?");
  Serial.print("  Msg: ");    Serial.print(count ? count : "?");
  Serial.print("  Temp: ");   Serial.print(temp  ? temp  : "?"); Serial.print(" C");
  Serial.print("  Moisture: "); Serial.print(moist ? moist : "?"); Serial.print(" %");
  Serial.print("  RSSI: ");   Serial.print(rssi);
  Serial.print("  SNR: ");    Serial.println(snr);
}
