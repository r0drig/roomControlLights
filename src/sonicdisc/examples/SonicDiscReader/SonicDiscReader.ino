#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Wire.h>
#include <stdint.h>
#include "Timer.h"


const uint8_t SONIC_DISC_I2C_ADDRESS = 0x09;
const uint8_t NUM_OF_SENSORS = 8; // No. of ultrasonic sensors on SonicDisc
// The packet contains NUM_OF_MEASUREMENTS measurements and an error code
const uint8_t I2C_PACKET_SIZE = NUM_OF_SENSORS + 1;
// The number of measurements from each sensor to filter
const uint8_t MEASUREMENTS_TO_FILTER = 5;
const uint8_t INT_PIN = 19;
const unsigned int VARIANCE_THRESHOLD = 10;// The max valid variance in a set of MEASUREMENTS_TO_FILTER measurements

//LDR sensor
const uint8_t LDRpin = A11;
uint16_t sensorValueLDR = 0;  // variable to store the value coming from the sensor

//
byte mac[] = {  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 2, 2);
IPAddress gateway(192, 168, 2, 1);
IPAddress submask(255, 255, 255, 0);
IPAddress ipDest(192, 168, 2, 254);
uint16_t localPort = 8888;      // local port to listen on
uint16_t destPort = 8888;      // local port to listen on

//class
Timer time;
EthernetUDP Udp;


// Sonic Disc's operational states
enum State {
  STANDBY, // MCU and sensors are on but no measurements are being made
  MEASURING // Sonic Disc is conducting measurements using the sensors
};

// Values to be received via I2C from master
enum I2C_RECEIPT_CODE {
  STATE_TO_STANDBY = 0x0A,
  STATE_TO_MEASURING = 0x0B
};

// Error codes to be transmitted via I2c to the master
enum I2C_ERROR_CODE {
  NO_ERROR,
  IN_STANDBY,
  INCOMPLETE_MEASUREMENT
};

// Flag to indicate the SonicDisc is ready to send a new set of data
volatile bool newData = false;

uint8_t filterIndex = 0;
uint8_t filterBuffer[MEASUREMENTS_TO_FILTER][NUM_OF_SENSORS] = {0};
uint8_t filteredMeasurements[NUM_OF_SENSORS] = {0};
bool newFilteredMeasurements = false;
//**********************************************************************************************************
/**
   Requests an I2C packet from the SonicDisc
   @param  i2cInput         The array that will hold the incoming packet
   @param  transmissionSize The size/length of the incoming packet
   @return                  Error code contained inside the incoming packet
*/
I2C_ERROR_CODE requestPacket(uint8_t i2cInput[], const uint8_t transmissionSize = I2C_PACKET_SIZE) {
  Wire.requestFrom(SONIC_DISC_I2C_ADDRESS, transmissionSize);
  uint8_t packetIndex = 0;
  while (Wire.available() && packetIndex < transmissionSize) {
    i2cInput[packetIndex++] = Wire.read();
  }
  return i2cInput[0]; // Return the packet's error code
}
//**********************************************************************************************************
/**
   Sends the supplied byte to the SonicDisc
   @param byteToSend The byte to be sent
*/
void sendData(uint8_t byteToSend) {
  Wire.beginTransmission(SONIC_DISC_I2C_ADDRESS);
  Wire.write(byteToSend);
  Wire.endTransmission(SONIC_DISC_I2C_ADDRESS);
}
//**********************************************************************************************************
/**
   ISR that raises a flag whenever SonicDisc is ready to transmit new data.
*/
void newSonicDiscData() {
  newData = true;
}
//**********************************************************************************************************
/*
   Adds the specified i2c packet in the buffer to be sorted later.
*/
void addInputToFilterBuffer(uint8_t i2cInput[], const uint8_t bufferIndex) {
  // Copy the whole packet (except error code) in the specified row of the buffer
  for (int i = 0, j = 1; i < NUM_OF_SENSORS; i++, j++) {
    filterBuffer[bufferIndex][i] = i2cInput[j];
  }
}
//**********************************************************************************************************
/*
    Sorts the measurements of each sensor for every cycle of measurements.
*/
void sortMeasurements() {
  // For each sensor
  for (int s = 0; s < NUM_OF_SENSORS; s++) {
    // Use bubble sort to sort all measurements throughout the cycle
    for (int i = 0; i < MEASUREMENTS_TO_FILTER - 1; i++) {
      for (int j = 0; j < MEASUREMENTS_TO_FILTER - i - 1; j++) {
        if (filterBuffer[j][s] > filterBuffer[j + 1][s]) {
          uint8_t toSwap = filterBuffer[j][s];
          filterBuffer[j][s] = filterBuffer[j + 1][s];
          filterBuffer[j + 1][s] = toSwap;
        }
      }
    }
  }
}
//**********************************************************************************************************
/*
   read and filter of ldr
   kalman filter
*/
uint16_t ldrSensor() {




  const int numReadings = 10;

  int readings[numReadings];      // the readings from the analog input
  int readIndex = 0;              // the index of the current reading
  int total = 0;                  // the running total
  int average = 0;                // the average

  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead( LDRpin );
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex ++;

  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }

  // calculate the average:
  average = total / numReadings;
  return analogRead( LDRpin );
}
//**********************************************************************************************************
/*
    Filter measurements depending on the variance.
    If variance is too high for a MEASUREMENTS_TO_FILTER measurements of a sensor
    then these measurements are disregarded. Otherwise the mean value is chosen.
*/
void filterMeasurements() {
  // Go through all the measurements taken for each sensor
  for (int i = 0; i < NUM_OF_SENSORS; i++) {
    // Calculate the variance across the different measurements
    // by subtracting the first and the last element
    // of the *sorted* measurement cycle.
    int variance = filterBuffer[0][i] - filterBuffer[MEASUREMENTS_TO_FILTER - 1][i];
    if (abs(variance) > VARIANCE_THRESHOLD) {
      filteredMeasurements[i] = 0;
    } else {
      filteredMeasurements[i] = filterBuffer[MEASUREMENTS_TO_FILTER / 2][i];
    }
  }
}
//**********************************************************************************************************
/*
   envio de informação
*/

void takeSendingMessage()
{
  newFilteredMeasurements = true;
}
//**********************************************************************************************************
void setup() {
  //i2c
  Wire.begin();

  //ethernet
  Ethernet.init(10);  // Most Arduino shields
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(INT_PIN), newSonicDiscData, RISING); //interrupt extern
  
  Serial.println("Requesting packet from SonicDisc");
  uint8_t dummyInput[I2C_PACKET_SIZE] = {0}; // A throw-away array
  // Do not proceed unless the SonicDisc is in "MEASURING" state
  while (requestPacket(dummyInput, I2C_PACKET_SIZE) == IN_STANDBY) {
    Serial.println("Setting state to MEASURING");
    sendData(STATE_TO_MEASURING);
  }
  Serial.println("Communication is established and SonicDisc is measuring distances");

  
 //config timer for send message
  time.every(1000, takeSendingMessage, &newFilteredMeasurements );
}
//**********************************************************************************************************
void loop() {
  //Serial.println("newData");
  time.update();//update time
  if (newData) {
    newData = false; // Indicate that we have read the latest data

    uint8_t sonicDiscInput[I2C_PACKET_SIZE] = {0};
    // Get the I2C packet
    I2C_ERROR_CODE ret = requestPacket(sonicDiscInput, I2C_PACKET_SIZE);
    // Now sonicDiscInput contains the latest measurements from the sensors.
    // However we need to make sure that the data is also of good quality
    // Process the packet only if it is a valid one
    if (ret == NO_ERROR) {
      addInputToFilterBuffer(sonicDiscInput, filterIndex);
      // When we have filled up the filter buffer, time to filter the measurements
      if (filterIndex + 1 == MEASUREMENTS_TO_FILTER) {
        // For each measurement
        sortMeasurements();
        filterMeasurements();
        // Indicate that the measurements are filtered
        newFilteredMeasurements = true;
      }
      // Move along the index
      filterIndex = (filterIndex + 1) % MEASUREMENTS_TO_FILTER;
    }
    sensorValueLDR = ldrSensor();
  }

//mesage
  if (newFilteredMeasurements)
  {

    Udp.beginPacket(ipDest, destPort);
    // Print the measurements nicely
    Serial.print("S");
    Udp.write("S");
    for (int i = 0; i < NUM_OF_SENSORS; i++) {
      Serial.print(",");
      Serial.print(filteredMeasurements[i]);
      Udp.write(filteredMeasurements[i]);
    }

    Udp.write(sensorValueLDR);
    Udp.endPacket();

    Serial.print(",");
    Serial.println(sensorValueLDR);
    newFilteredMeasurements = false; //flag de nova lentura
  }
}
