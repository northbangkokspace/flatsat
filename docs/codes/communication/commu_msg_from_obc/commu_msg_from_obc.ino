HardwareSerial obc_uart(PA12,PA11);
void setup(){
    obc_uart.begin(115200);
    Serial.setTx(PA2);
    Serial.setRx(PA3);
    Serial.begin(115200);
    Serial.println("Hello from Commu!");
}

void loop(){
    if (obc_uart.available()){
        String msg = obc_uart.readStringUntil('\n');
        Serial.print("Received message from OBC: ");
        Serial.println(msg);
        int num = msg.toInt();
        Serial.print("Sent message to OBC: ");
        Serial.println(num);
        obc_uart.println(num*2);
    }

    while(Serial.available()){
        String msg = Serial.readStringUntil('\n');
        int num = msg.toInt();
        obc_uart.println(num);
        Serial.print("Sent message to OBC: ");
        Serial.println(msg);
    }
}