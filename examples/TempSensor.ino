int sensor = A1;
float reader;


void setup() {
  Serial.begin(9600);

}

void loop() {
  reader = analogRead(sensor);

  // converting that reading to voltage
  float voltage = reader * 5.0;
  voltage /= 1024.0;

  // print out the voltage
  Serial.print(voltage);
  Serial.println(" volts");

  // now print out the temperature
  float celsius = (voltage - 0.5) * 100 ;

  Serial.println("The temperature is: ");
  Serial.print(celsius);
  Serial.println(" degree Celsius");
  Serial.println();

  delay(2000);

}
