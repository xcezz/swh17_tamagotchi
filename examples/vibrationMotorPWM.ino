void setup() {
  pinMode(6, OUTPUT);  // Must be a PWM pin
}

void loop() {
  analogWrite(6, 153);  // 60% duty cycle
  delay(1000);

}
