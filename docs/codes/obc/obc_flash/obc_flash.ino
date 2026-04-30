#include "SdFat_Adafruit_Fork.h"
#include <SPI.h>


// for flashTransport definition
#define EXTERNAL_FLASH_USE_SPI 1
#include <Adafruit_SPIFlash.h>
SPIClass SD_SPI(PB15, PB14, PB13);
Adafruit_FlashTransport_SPI flashTransport(PB12, SD_SPI);
Adafruit_SPIFlash flash(&flashTransport);
uint8_t data[4096];

void dump_sector(uint32_t sector) {
  uint8_t buf[4096];
  memset(buf, 0xff, sizeof(buf));

  flash.readSector(sector, buf);

  for (uint32_t row = 0; row < sizeof(buf) / 16; row++) {
    if (row == 0)
      Serial.print("0");
    if (row < 16)
      Serial.print("0");
    Serial.print(row * 16, HEX);
    Serial.print(" : ");

    for (uint32_t col = 0; col < 16; col++) {
      uint8_t val = buf[row * 16 + col];

      if (val < 16)
        Serial.print("0");
      Serial.print(val, HEX);

      Serial.print(" ");
    }
    Serial.println();
  }
}

void setup() {
  Serial.setTx(PD8);
  Serial.setRx(PD9);
  Serial.begin(115200);

  Serial.println("Adafruit Serial Flash Info example");
  flash.begin();

  uint32_t jedec_id = flash.getJEDECID();
  Serial.print("JEDEC ID: 0x");
  Serial.println(jedec_id, HEX);
}


void loop() {
  int size = 256;
  // flash.eraseSector(0);
  for (int i = 0; i < size; i++) {
    data[i] = random(0,255);
  }
  flash.writeSector(0, data);
  Serial.println("Written");
  for (int i = 0; i < (size / 16); i++){
    for (int j = 0; j < 16; j++){
      Serial.printf("%02X ",data[(i * 16) + j]);
    }
    Serial.println();
  }
  uint8_t tmp[4096];
  Serial.println("Read");
  flash.readSector(0, tmp);
  for (int i = 0; i < (size / 16); i++){
    for (int j = 0; j < 16; j++){
      Serial.printf("%02X ",tmp[(i * 16) + j]);
    }
    Serial.println();
  }
  Serial.println("---------------000---------------");
  delay(5000);
  // nothing to do
}
