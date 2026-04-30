HardwareSerial gps_uart(PE0,PE1);
void setup(){
    gps_uart.begin(9600);
    Serial.setTx(PD8);
    Serial.setRx(PD9);
    Serial.begin(115200);
    Serial.println("Hello from OBC!");
}

void loop(){
    while (gps_uart.available()){
        char c = gps_uart.read();
        Serial.write(c);
    }
}