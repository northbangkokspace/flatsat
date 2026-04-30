/*
 * Unified OBC Flatsat Test Code
 * Tests GPIO, UART, I2C, SPI, SD Card, EPS Sensors, and Camera sequentially.
 */

#include <Wire.h>
#include <SPI.h>
#include "SdFat.h"
#include <Adafruit_SPIFlash.h>
#include "Arducam_Mega.h"
#include <stdint.h>

// ==============================================================================
// 1. PIN DEFINITIONS
// ==============================================================================
// Power Control
#define PWR_PD0 PD0
#define PWR_PD1 PD1
#define PWR_PD2 PD2
#define PWR_PD3 PD3

// Commu UART
HardwareSerial commu_uart(PA1, PA0);

// I2C Buses
TwoWire INTERNAL_I2C(PB9, PB8);  // SDA, SCL
TwoWire EPS_SEN(PF0, PF1);       // SDA, SCL

// SPI Flash
#define EXTERNAL_FLASH_USE_SPI 1
SPIClass FLASH_SPI(PB15, PB14, PB13);
Adafruit_FlashTransport_SPI flashTransport(PB12, FLASH_SPI);
Adafruit_SPIFlash flash(&flashTransport);

// SD Card
#define SPI_DRIVER_SELECT 2
#define ENABLE_DEDICATED_SPI 1
SPIClass SD_SPI(PC12, PC11, PC10);
const int SD_CS = PC9;
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(8), &SD_SPI)
SdFs sd;
FsFile file;
csd_t csd;

// Camera
const int CAM_CS = PE_7;
Arducam_Mega myCAM(CAM_CS);
#define BUFFER_SIZE 0xff

// ==============================================================================
// 2. EPS SENSOR CLASSES & HARDWARE CONFIG
// ==============================================================================
class I2CDevice {
protected:
  TwoWire& wire;
  uint8_t address;
public:
  I2CDevice(TwoWire& wireInstance, uint8_t addr)
    : wire(wireInstance), address(addr) {}

  void writeRegister16(uint8_t reg, uint16_t value) {
    wire.beginTransmission(address);
    wire.write(reg);
    wire.write((value >> 8) & 0xFF);
    wire.write(value & 0xFF);
    wire.endTransmission();
  }

  void writePointer(uint8_t reg) {
    wire.beginTransmission(address);
    wire.write(reg);
    wire.endTransmission();
  }

  uint16_t readRegister16(uint8_t reg) {
    wire.beginTransmission(address);
    wire.write(reg);
    wire.endTransmission(false);
    wire.requestFrom(address, (uint8_t)2);
    if (wire.available() >= 2) {
      uint16_t value = (uint16_t)wire.read() << 8;
      value |= wire.read();
      return value;
    }
    return 0;
  }

  uint8_t readBytes(uint8_t* buf, uint8_t len) {
    wire.requestFrom(address, len);
    uint8_t i = 0;
    while (wire.available() && i < len) {
      buf[i++] = wire.read();
    }
    return i;
  }
};

class INA226 : public I2CDevice {
public:
  INA226(TwoWire& wireInstance, uint8_t addr = 0x40)
    : I2CDevice(wireInstance, addr) {}
  void calibrate(float rShunt_Ohms, float maxExpectedCurrent_Amps) {
    currentLSB = maxExpectedCurrent_Amps / 32768.0f;
    uint16_t calValue = (uint16_t)(0.00512f / (currentLSB * rShunt_Ohms));
    writeRegister16(0x05, calValue);
  }
  float getBusVoltage_V() {
    return readRegister16(0x02) * 0.00125f;
  }
  float getShuntVoltage_mV() {
    return (int16_t)readRegister16(0x01) * 0.0025f;
  }
  float getCurrent_A() {
    return (currentLSB == 0.0f) ? 0.0f : ((int16_t)readRegister16(0x04) * currentLSB);
  }
private:
  float currentLSB = 0.0f;
};

class TMP102 : public I2CDevice {
public:
  TMP102(TwoWire& wireInstance, uint8_t addr = 0x48)
    : I2CDevice(wireInstance, addr) {}
  float readTemperatureC() {
    return ((int16_t)readRegister16(0x00) >> 4) * 0.0625f;
  }
};

class ADM1177 : public I2CDevice {
public:
  ADM1177(TwoWire& wireInstance, float rSenseOhms, uint8_t addr = 0x2D)
    : I2CDevice(wireInstance, addr), rSense(rSenseOhms) {}
  void startConversion() {
    // Datasheet Fix: writePointer uses a single byte instead of a register+command byte.
    // 7:2 voltage divider flag safely set with (1 << 4).
    writePointer(0b00000001 | 0b00000100 | (1 << 4));
  }
  bool readData(float& voltage, float& current) {
    uint8_t buf[3] = { 0 };
    if (readBytes(buf, 3) < 3) return false;

    // Byte 1 & 2 MSBs, Byte 3 LSBs
    uint16_t vRaw = ((uint16_t)(buf[0] << 4)) | (buf[2] >> 4);
    uint16_t iRaw = ((uint16_t)(buf[1] << 4)) | (buf[2] & 0x0F);

    voltage = vRaw * (6.65f / 4096.0f);
    current = iRaw * (0.10584f / (4096.0f * rSense));
    return true;
  }
private:
  float rSense;
};

// EPS Device Instances
INA226 powerMonitors[] = {
  INA226(EPS_SEN, 0x40),
  INA226(EPS_SEN, 0x41),
  INA226(EPS_SEN, 0x42),
  INA226(EPS_SEN, 0x43),
  INA226(EPS_SEN, 0x47),
  INA226(EPS_SEN, 0x48),
};
TMP102 tempSensors[] = { TMP102(EPS_SEN, 0x4A), TMP102(EPS_SEN, 0x4B) };
ADM1177 powerControllers[] = {
  ADM1177(EPS_SEN, 0.06f, 0x58),
  ADM1177(EPS_SEN, 0.06f, 0x59),
  ADM1177(EPS_SEN, 0.06f, 0x5A),
  ADM1177(EPS_SEN, 0.06f, 0x5B),
};

const int INA226_COUNT = 6;
const int TMP102_COUNT = 2;
const int ADM1177_COUNT = 4;


// ==============================================================================
// 3. SEQUENTIAL TEST FUNCTIONS
// ==============================================================================

void testPowerControl() {
  Serial.println("\r\n[1] Testing Power Control Pins...");
  pinMode(PWR_PD0, OUTPUT);
  pinMode(PWR_PD1, OUTPUT);
  pinMode(PWR_PD2, OUTPUT);
  pinMode(PWR_PD3, OUTPUT);

  for (int i = 0; i < 4; i++) {
      digitalWrite(PWR_PD0, LOW);
      digitalWrite(PWR_PD1, LOW);
      digitalWrite(PWR_PD2, LOW);
      digitalWrite(PWR_PD3, LOW);
      Serial.println("  -> Power control pins PD0-PD3 set LOW.");
      delay(500);
      digitalWrite(PWR_PD2, HIGH);
      digitalWrite(PWR_PD3, HIGH);
      digitalWrite(PWR_PD0, HIGH);
      digitalWrite(PWR_PD1, HIGH);
      Serial.println("  -> Power control pins PD0-PD3 set HIGH.");
      delay(500);
  }
}

void testCommuUART() {
  Serial.println("\r\n[2] Testing Commu UART...");
  commu_uart.begin(115200);
  commu_uart.println("OBC TEST");
  Serial.println("  -> Sent 'OBC TEST' to Commu UART (PA1/PA0).");
}

void scanI2CBus(TwoWire& wireBus, const char* busName) {
  Serial.printf("  -> Scanning %s...\r\n", busName);
  byte error, address;
  int nDevices = 0;
  for (address = 1; address < 127; address++) {
    wireBus.beginTransmission(address);
    error = wireBus.endTransmission();
    if (error == 0) {
      Serial.printf("     Found device at 0x%02X\r\n", address);
      nDevices++;
    }
  }
  if (nDevices == 0) Serial.println("     No I2C devices found.");
}

void testI2CBuses() {
  Serial.println("\r\n[3] Testing Internal and EPS I2C Buses...");
  INTERNAL_I2C.begin();
  EPS_SEN.begin();
  scanI2CBus(INTERNAL_I2C, "Internal I2C (PB9/PB8)");
  scanI2CBus(EPS_SEN, "EPS I2C (PF0/PF1)");
}

void testFlashSPI() {
  Serial.println("\r\n[4] Testing SPI Flash...");
  if (flash.begin()) {
    uint32_t jedec_id = flash.getJEDECID();
    Serial.printf("  -> Flash initialized! JEDEC ID: 0x%X\r\n", jedec_id);
  } else {
    Serial.println("  -> Flash initialization failed.");
  }
}

void testSDCard() {
  Serial.println("\r\n[5] Testing SD Card...");
  if (!sd.begin(SD_CONFIG)) {
    Serial.println("  -> SD initialization failed! Check card insertion and wiring.");
  } else {
    Serial.print("  -> SD initialized. Card type: ");
    switch (sd.card()->type()) {
      case SD_CARD_TYPE_SD1: Serial.println("SD1"); break;
      case SD_CARD_TYPE_SD2: Serial.println("SD2"); break;
      case SD_CARD_TYPE_SDHC: Serial.println("SDHC"); break;
      default: Serial.println("Unknown");
    }
  }
}

void testEPSSensors() {
  Serial.println();
  Serial.println("[6] Reading EPS Sensors...");

  // Calibrate INA226 sensors first
  for (int i = 0; i < INA226_COUNT; i++) {
    powerMonitors[i].calibrate(0.02f, 4.096f);
    Serial.print("  -> INA226[");
    Serial.print(i);
    Serial.print("]: Bus Voltage: ");
    Serial.print(powerMonitors[i].getBusVoltage_V(), 3);  // 3 decimal places
    Serial.print(" V, Current: ");
    Serial.print(powerMonitors[i].getCurrent_A(), 3);
    Serial.println(" A");
  }

  for (int i = 0; i < TMP102_COUNT; i++) {
    Serial.print("  -> TMP102[");
    Serial.print(i);
    Serial.print("]: Temperature: ");
    Serial.print(tempSensors[i].readTemperatureC(), 2);  // 2 decimal places
    Serial.println(" C");
  }

  for (int i = 0; i < ADM1177_COUNT; i++) {
    powerControllers[i].startConversion();
    delay(10);  // Wait for conversion over I2C
    float voltage, current;
    if (powerControllers[i].readData(voltage, current)) {
      Serial.print("  -> ADM1177[");
      Serial.print(i);
      Serial.print("]: V: ");
      Serial.print(voltage, 3);
      Serial.print(" V, I: ");
      Serial.print(current, 3);
      Serial.println(" A");
    } else {
      Serial.print("  -> ADM1177[");
      Serial.print(i);
      Serial.println("]: Read Failed");
    }
  }
}

void testCameraToSD() {
  Serial.println("\r\n[7] Testing Camera & Saving to SD...");
  // Must override pins dynamically specifically for the Arducam SPI bus configuration
  SPI.setMISO(PB_4);
  SPI.setMOSI(PB_5);
  SPI.setSCLK(PB_3);
  SPI.begin();

  myCAM.begin();
  Serial.println("  -> Taking picture...");
  myCAM.takePicture(CAM_IMAGE_MODE_VGA, CAM_IMAGE_PIX_FMT_JPG);
  delay(500);
  myCAM.takePicture(CAM_IMAGE_MODE_VGA,CAM_IMAGE_PIX_FMT_JPG);


  uint8_t imageBuff[BUFFER_SIZE] = { 0 };
  unsigned int i = 0;
  uint8_t headFlag = 0;
  uint8_t imageData = 0, imageDataNext = 0;

  if (myCAM.getReceivedLength()) {
    if (file.open("test.jpg", O_RDWR | O_CREAT)) {
      Serial.println("  -> Saving to test.jpg on SD Card...");
      while (myCAM.getReceivedLength()) {
        imageData = imageDataNext;
        imageDataNext = myCAM.readByte();

        if (headFlag == 1) {
          imageBuff[i++] = imageDataNext;
          if (i >= BUFFER_SIZE) {
            file.write(imageBuff, i);
            i = 0;
          }
        }
        if (imageData == 0xff && imageDataNext == 0xd8) {
          headFlag = 1;
          imageBuff[i++] = imageData;
          imageBuff[i++] = imageDataNext;
        }
        if (imageData == 0xff && imageDataNext == 0xd9) {
          headFlag = 0;
          file.write(imageBuff, i);
          i = 0;
          break;
        }
      }
      file.close();
      Serial.println("  -> Image saved successfully.");
      sd.end();
    } else {
      Serial.println("  -> Failed to open file on SD.");
    }
  } else {
    Serial.println("  -> No image data received from camera.");
  }
}

// ==============================================================================
// 4. MAIN ENTRY POINTS
// ==============================================================================
void setup() {
  Serial.setTx(PD8);
  Serial.setRx(PD9);
  Serial.begin(115200);
  delay(2000);  // Allow time for monitor to open

  Serial.println("=========================================");
  Serial.println("   KMITL Flatsat Unified Test Routine    ");
  Serial.println("=========================================");

  testPowerControl();
  testCommuUART();
  testI2CBuses();
  testFlashSPI();
  testSDCard();
  testEPSSensors();
  testCameraToSD();

  Serial.println("\r\n=========================================");
  Serial.println("          All Tests Completed            ");
  Serial.println("=========================================");
}

void loop() {
  // End of tests. Halt.
  while (1) {
    delay(1000);
  }
}