#include <Wire.h>
// Override weak default SDA pin mapping with internal pull-up
// const PinMap PinMap_I2C_SDA[] = {
//   { PF_0, I2C1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF4_I2C2) },
//   { NC, NP, 0 }
// };

// // Override weak default SCL pin mapping with internal pull-up
// const PinMap PinMap_I2C_SCL[] = {
//   { PF_1, I2C1, STM_PIN_DATA(STM_MODE_AF_OD, GPIO_PULLUP, GPIO_AF4_I2C2) },
//   { NC, NP, 0 }
// };

void setup() {
  Wire.setSCL(PF1);
  Wire.setSDA(PF0);
  Wire.begin();
  Serial.setTx(PD8);
  Serial.setRx(PD9);
  Serial.begin(115200);
  Serial.println("\nI2C Scanner");
}

void loop() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(2000);  // wait 5 seconds for next scan
}