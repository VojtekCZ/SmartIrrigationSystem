#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <BH1750.h>
#include <NewPing.h>

// Sensor definitions
#define BME280_ADDRESS (0x76)
Adafruit_BME280 bme;             // BME280 sensor for temperature, pressure, and humidity
BH1750 lightMeter;               // BH1750 light intensity sensor

// Pin configuration
#define soilSensorPin A0          // Analog pin for soil moisture sensor
#define relayPumpPin 6            // Relay pin for the water pump
#define relayLightPin 13          // Relay pin for the light
#define trigPin 5                 // Trigger pin for ultrasonic sensor
#define echoPin 2                 // Echo pin for ultrasonic sensor
#define maxDistance 450           // Maximum range of ultrasonic sensor (cm)

// LCD setup: (rs, enable, d4, d5, d6, d7)
LiquidCrystal lcd(12, 11, 10, 9, 8, 7);

// Ultrasonic sensor
NewPing sonar(trigPin, echoPin, maxDistance);

// Thresholds for sensors
const int soilThresholdLow = 600;  // Soil moisture level to turn off the pump
const int soilThresholdHigh = 700; // Soil moisture level to turn on the pump
const int lightThreshold = 200;    // Light intensity threshold (lux) to control lighting
const int waterLevelThreshold = 20; // Minimum water level distance (cm) before alerting

// State variables
bool isPumpRunning = false;        // State of the pump
unsigned long previousDisplayMillis = 0;
const unsigned long displayInterval = 1000; // LCD update interval (1 second)
int displayState = 0;                        // Current LCD display mode

void setup() {
  // Initialize LCD
  lcd.begin(16, 2);
  lcd.clear();

  // Initialize pin modes
  pinMode(relayPumpPin, OUTPUT);
  pinMode(relayLightPin, OUTPUT);
  pinMode(soilSensorPin, INPUT);

  // Start serial communication
  Serial.begin(9600);

  // Initialize BME280 sensor
  if (!bme.begin(BME280_ADDRESS)) {
    Serial.println("ERROR: BME280 sensor not found!");
    while (1); // Halt if sensor initialization fails
  }

  // Initialize BH1750 sensor
  if (!lightMeter.begin()) {
    Serial.println("ERROR: BH1750 sensor not found!");
    while (1); // Halt if sensor initialization fails
  }
  lightMeter.configure(BH1750::CONTINUOUS_HIGH_RES_MODE); // Set high-resolution mode
}

void loop() {
  unsigned long currentMillis = millis();

  // Read sensor data
  int soilMoisture = analogRead(soilSensorPin);
  float temperature = bme.readTemperature(); // Temperature in °C
  float pressure = bme.readPressure() / 100.0F; // Atmospheric pressure in hPa
  float humidity = bme.readHumidity();         // Relative humidity in %
  uint16_t lux = lightMeter.readLightLevel();  // Light intensity in lux
  int waterLevel = getWaterLevel();            // Water level in cm

  // Print sensor values to the serial monitor
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Pressure: "); Serial.print(pressure); Serial.println(" hPa");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Light Intensity: "); Serial.print(lux); Serial.println(" lx");
  Serial.print("Soil Moisture: "); Serial.print(soilMoisture); Serial.println(" (ADC)");
  Serial.print("Water Level: "); Serial.print(waterLevel); Serial.println(" cm");
  Serial.println("-----------------------");

  // Control the pump with hysteresis logic
  if (!isPumpRunning && soilMoisture > soilThresholdHigh) {
    digitalWrite(relayPumpPin, LOW); // Turn on the pump
    isPumpRunning = true;
    Serial.println("Pump: ON");
  } else if (isPumpRunning && soilMoisture <= soilThresholdLow) {
    digitalWrite(relayPumpPin, HIGH); // Turn off the pump
    isPumpRunning = false;
    Serial.println("Pump: OFF");
  }

  // Control the light relay
  if (lux < lightThreshold) {
    digitalWrite(relayLightPin, LOW); // Turn on the light
    Serial.println("Light: ON");
  } else {
    digitalWrite(relayLightPin, HIGH); // Turn off the light
    Serial.println("Light: OFF");
  }

  // Update the LCD display every second
  if (currentMillis - previousDisplayMillis >= displayInterval) {
    previousDisplayMillis = currentMillis;
    displayState = (displayState + 1) % 3; // Cycle through display modes
    updateDisplay(displayState, temperature, pressure, humidity, lux, soilMoisture, waterLevel);
  }
}

// Function to update the LCD display based on the current mode
void updateDisplay(int state, float temperature, float pressure, float humidity, uint16_t lux, int soilMoisture, int waterLevel) {
  lcd.clear();
  switch (state) {
    case 0: // Display temperature and pressure
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature);
      lcd.print(" C");

      lcd.setCursor(0, 1);
      lcd.print("Press: ");
      lcd.print(pressure);
      lcd.print(" hPa");
      break;

    case 1: // Display humidity and light intensity
      lcd.setCursor(0, 0);
      lcd.print("Humidity: ");
      lcd.print(humidity);
      lcd.print(" %");

      lcd.setCursor(0, 1);
      lcd.print("Light: ");
      lcd.print(lux);
      lcd.print(" lx");
      break;

    case 2: // Display water level and soil moisture
      lcd.setCursor(0, 0);
      lcd.print("Water Lvl: ");
      lcd.print(waterLevel);
      lcd.print(" cm");

      lcd.setCursor(0, 1);
      lcd.print("Soil: ");
      lcd.print(soilMoisture);
      lcd.print(" ADC");
      break;
  }
}

// Function to measure the water level using an ultrasonic sensor
int getWaterLevel() {
  unsigned int distanceSum = 0;

  // Take 5 measurements for averaging
  for (int i = 0; i < 5; i++) {
    int distance = sonar.ping_cm();
    if (distance > 0) {
      distanceSum += distance;
    }
    delay(50); // Wait between measurements
  }

  // Return maximum distance if no valid measurement
  if (distanceSum == 0) {
    return maxDistance;
  }

  // Return the average distance
  return distanceSum / 5;
}
