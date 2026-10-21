//Reading HIH-4030 Humidity Sensors(discontinued but available for checkout at ITLL) using an Arduino Uno

int sensorPins[] = {A0, A1};
int pinCount = sizeof(sensorPins) / sizeof(sensorPins[0]);

void setup() {
  Serial.begin(9600);

}

void loop() {
  //  Serial.print("this is sizeof(sensorPins)");
  //  Serial.println(sizeof(sensorPins[0]));

  //  for (int i = 0; i  < pinCount; i++) {
  //    float data = analogRead(sensorPins[i]) * 5.000 / 1023.000 * 40 / 2;
  //    Serial.println(data);
  //    delay(100);

  Serial.print(relHumidity(A0));
  Serial.print(",");
  Serial.println(relHumidity(A5));
  delay(50);


}


float relHumidity (int pin) {

  float humidpercent = analogRead(pin) * 5.000 / 1023.000 * 40 / 2; //converting bits to voltage, then to RH% from this datasheet: https://www.pololu.com/product/1643
  return humidpercent;

}
