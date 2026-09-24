/*
 * NBSPACE Labs: FlatSat Learning Set
 * Board Test: Unified TX/RX Test with Dual LEDs
 * Feature to be tested: LEDs (Active LOW), RF Ping-Pong, UART
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// ====================================================================
// HARDWARE PIN DEFINITIONS 
// ====================================================================
// --- Radio Pins ---
#define RADIO_SCK   PA5
#define RADIO_MISO  PA6
#define RADIO_MOSI  PA7
#define RADIO_NSS   PB6
#define RADIO_DIO0  PA10
#define RADIO_RESET PC7
#define RADIO_DIO1  -1

HardwareSerial obc_uart(PA12,PA11);

// --- LED Pins ---
// CHANGE THESE TO MATCH YOUR ACTUAL LED PINS
#define LED_TX PB13
#define LED_RX PB12 

SX1278 radio = new Module(RADIO_NSS, RADIO_DIO0, RADIO_RESET, RADIO_DIO1, SPI);

// ====================================================================
// INTERRUPT FLAG & ISR 
// ====================================================================
volatile bool receivedFlag = false;

void setFlag(void) {
  receivedFlag = true;
}

// ====================================================================
// TIMING VARIABLES
// ====================================================================
uint32_t lastTxTime = 0;
uint32_t txInterval = 3000; // Base interval: 3 seconds

// ====================================================================
// SETUP FUNCTION
// ====================================================================
void setup() {
  // 1. Initialize LEDs (Active LOW: HIGH = OFF, LOW = ON)
  pinMode(LED_TX, OUTPUT);
  pinMode(LED_RX, OUTPUT);
  digitalWrite(LED_TX, HIGH); // Turn OFF Transmit LED initially
  digitalWrite(LED_RX, HIGH); // Turn OFF Receive LED initially

  // 2. Initialize UART
  Serial.begin(115200);
  obc_uart.begin(115200);
  delay(1000);
  
  Serial.println("\n=== FlatSat Board Test Started ===");
  Serial.println("Testing UART, RF, and LEDs (Active LOW)...");

  // 3. Initialize SPI & Radio
  SPI.setMISO(RADIO_MISO);
  SPI.setMOSI(RADIO_MOSI);
  SPI.setSCLK(RADIO_SCK);
  SPI.begin();

  if (radio.beginFSK() == RADIOLIB_ERR_NONE) {
    Serial.println("[SYSTEM] Radio Begin OK");
    radio.setFrequency(433.0);
    radio.setBitRate(9.6);
    radio.setOutputPower(2);

    // Bind hardware transceiver event to ISR flag
    radio.setDio0Action(setFlag, RISING);

    // Trigger background reception
    int state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("[SYSTEM] RF Interrupt enabled. Listening to the sky...");
    } else {
      Serial.print("[ERROR] Failed to start receive. Code: ");
      Serial.println(state);
    }
  } else {
    Serial.println("[ERROR] RF Module failed!");
    while (true);
  }

  // Seed random generator for collision avoidance
  randomSeed(analogRead(PA0)); 
}

// ====================================================================
// MAIN LOOP FUNCTION
// ====================================================================
void loop() {
  while (obc_uart.available()){
      Serial.write(obc_uart.read());
      digitalWrite(LED_TX,HIGH);
      digitalWrite(LED_RX,LOW);
      delay(60);
      digitalWrite(LED_TX,LOW);
      digitalWrite(LED_RX,HIGH);
      delay(60);
      digitalWrite(LED_TX,HIGH);
      digitalWrite(LED_RX,HIGH);
      delay(60);
      lastTxTime = millis();
      receivedFlag = false; 
      Serial.println("COMMU TEST");
  }
  // --------------------------------------------------------
  // TASK 1: BEACON TRANSMISSION
  // --------------------------------------------------------
  if (millis() - lastTxTime > txInterval) {
    // Set next interval with a slight random offset to prevent collisions 
    // if both boards power on simultaneously
    txInterval = 500 + random(0, 500); 
    lastTxTime = millis();

    digitalWrite(LED_TX, LOW);  // Turn ON Transmit LED (Active Low)
    Serial.println("[TX] Transmitting PING...");

    // radio.transmit is a blocking call by default.
    int state = radio.transmit("PING");

    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("[TX] Success!");
    } else {
      Serial.print("[ERROR] Transmit failed, code: ");
      Serial.println(state);
    }
    digitalWrite(LED_TX, HIGH); // Turn OFF Transmit LED (Active Low)

    // Re-enable receive mode & clear flag (transmit triggers the DIO0 interrupt)
    radio.startReceive();
    receivedFlag = false; 
  }

  // --------------------------------------------------------
  // TASK 2: RECEIVE HANDLING
  // --------------------------------------------------------
  if (receivedFlag) {
    receivedFlag = false; // Reset flag

    digitalWrite(LED_RX, LOW); // Turn ON Receive LED (Active Low)

    String rxString;
    int state = radio.readData(rxString);

    if (state == RADIOLIB_ERR_NONE) {
      Serial.print("[RX] Data Received: ");
      Serial.println(rxString);
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      Serial.println("[WARNING] RF CRC Mismatch! Bad signal received.");
    } else {
      Serial.print("[ERROR] Receive failed, code: ");
      Serial.println(state);
    }

    // Keep RX LED on just long enough to be visible to the human eye, then turn off
    delay(50); 
    digitalWrite(LED_RX, HIGH); // Turn OFF Receive LED (Active Low)

    // Reactivate non-blocking hardware receiver state
    radio.startReceive();
    receivedFlag = false;
  }
}