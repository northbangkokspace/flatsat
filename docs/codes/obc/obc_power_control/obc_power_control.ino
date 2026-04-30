void setup() {
  // put your setup code here, to run once:
  pinMode(PD0,OUTPUT);
  pinMode(PD1,OUTPUT);
  pinMode(PD2,OUTPUT);
  pinMode(PD3,OUTPUT);
}

void loop() {
  digitalWrite(PD0,!digitalRead(PD0));
  digitalWrite(PD1,!digitalRead(PD1));
  digitalWrite(PD2,!digitalRead(PD2));
  digitalWrite(PD3,!digitalRead(PD3));
  delay(1000);
  // put your main code here, to run repeatedly:

}
