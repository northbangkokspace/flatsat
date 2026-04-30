HardwareSerial commu_uart(PA1,PA0);
void setup(){
    commu_uart.begin(115200);
    Serial.setTx(PD8);
    Serial.setRx(PD9);
    Serial.begin(115200);
    Serial.println("Hello from OBC!");
}

void loop(){
    while (Serial.available()){
        String msg = Serial.readStringUntil('\n');
        int num = msg.toInt();
        commu_uart.println(num);
        Serial.print("Sent message to Commu: ");
        Serial.println(msg);
    }

    while (commu_uart.available()){
        String msg = commu_uart.readStringUntil('\n');
        Serial.print("Received message from Commu: ");
        Serial.println(msg);
    }
}