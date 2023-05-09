/* 
 *   Arduino or ESP32 Digital Level with LEDS
 *   
 *   Copyright (C) 2023   <<bitwise_gamgee>>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *   Parts List
 *   ESP32 development board
 *   MPU-6050 accelerometer and gyroscope module
 *   Breadboard (optional, but helpful for prototyping)
 *   Jumper wires
 *   20 LEDs
 *   20 resistors (e.g., 220 Ohm)
 *
 *   Wiring Instructions:
 *   
 *   MPU-6050 VCC to ESP32 3.3V
 *   MPU-6050 GND to ESP32 GND
 *   MPU-6050 SDA to ESP32 SDA (usually GPIO21)
 *   MPU-6050 SCL to ESP32 SCL (usually GPIO22)
 *     
 *   Connect the anode (longer leg) of each LED to a GPIO pin through a resistor
 *   Connect the cathode (shorter leg) of each LED to the ESP32 GND
*/

// Programmer note: 
// Floats are used in this program due to their 4byte space, if memory is not a concern, 
// you may change them all to doubles for slightly greater precision at the expense of computational performance.

#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// Define LED pins
const int ledPins[] = {12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33, 34, 35, 36, 39};
const int numLeds = sizeof(ledPins) / sizeof(ledPins[0]);

// Calculate Radians
const float radToDeg = 180.0 / PI;

// Low-pass filter constant
const float alpha = 0.1;

void setup() {
  Wire.begin();
  Serial.begin(115200);

  while (!mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_2G)) {
    Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
    delay(500);
  }

  // Initialize LED pins as outputs
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
  }
}

void loop() {
  float ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  // Initialize static variables for filtered accelerometer readings
  static float ax_filtered = ax;
  static float ay_filtered = ay;
  static float az_filtered = az;

  // Apply low-pass filter
  ax_filtered = alpha * ax + (1 - alpha) * ax_filtered;
  ay_filtered = alpha * ay + (1 - alpha) * ay_filtered;
  az_filtered = alpha * az + (1 - alpha) * az_filtered;

  float common_denominator_ay_az = ay_filtered * ay_filtered + az_filtered * az_filtered;
  float common_denominator_ax_az = ax_filtered * ax_filtered + az_filtered * az_filtered;
  float pitch = atan2(ax_filtered, sqrt(common_denominator_ay_az)) * radToDeg;
  float roll = atan2(ay_filtered, sqrt(common_denominator_ax_az)) * radToDeg;

  Serial.print("Pitch: ");
  Serial.print(pitch);
  Serial.print(" Roll: ");
  Serial.println(roll);

  // Calculate the number of LEDs to turn on based on the pitch
  int pitchLeds = map(abs(pitch), 0, 90, 0, numLeds);

  // Initialize static variable for the previous number of lit LEDs
  static int prevPitchLeds = pitchLeds;

  // Update only the LEDs that changed their state
  for (int i = 0; i < numLeds; i++) {
    if ((i < pitchLeds && i >= prevPitchLeds) || (i >= pitchLeds && i < prevPitchLeds)) {
      digitalWrite(ledPins[i], i < pitchLeds ? HIGH : LOW);
    }
  }


  // Update the previous number of lit LEDs
  prevPitchLeds = pitchLeds;

  delay(50);
}
