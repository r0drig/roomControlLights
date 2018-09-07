#include "commands.h"

#define PIN_LIGHTS_GROUP1   2
#define PIN_LIGHTS_GROUP2   3

char g_ultraSoundValues[] = {1, 2, 3, 4, 5, 6, 7, 8}; // 8 sensors, each one with range 0-256
char g_lightSensor = 1;
char g_pirSensor = 1;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_LIGHTS_GROUP1, OUTPUT);
  pinMode(PIN_LIGHTS_GROUP2, OUTPUT);
  Serial.begin(9600);
  digitalWrite(PIN_LIGHTS_GROUP1, LOW);
  digitalWrite(PIN_LIGHTS_GROUP2, LOW);
}

void loop() {
  int incomingByte = 0;
  
  digitalWrite(LED_BUILTIN, LOW);
  if ( Serial.available() > 0 )
  {
    incomingByte = Serial.read();

    switch ( incomingByte )
    {
      case SERIAL_COMMAND_ULTRASOUND:
        Serial.write(g_ultraSoundValues, sizeof(g_ultraSoundValues));
        break;
      case SERIAL_COMMAND_LIGHT:
        Serial.write(g_lightSensor);
        break;
      case SERIAL_COMMAND_PIR:
        Serial.write(g_pirSensor);
        break;
      case SERIAL_COMMAND_ALL:
        Serial.write(g_ultraSoundValues, sizeof(g_ultraSoundValues));
        Serial.write(g_lightSensor);
        Serial.write(g_pirSensor);
        break;
      case SERIAL_COMMAND_VOICE_LIGHTSON:
        digitalWrite(PIN_LIGHTS_GROUP1, HIGH);
        digitalWrite(PIN_LIGHTS_GROUP2, HIGH);
        Serial.write(incomingByte);
        break;
      case SERIAL_COMMAND_VOICE_LIGHTSOFF:
        digitalWrite(PIN_LIGHTS_GROUP1, LOW);
        digitalWrite(PIN_LIGHTS_GROUP2, LOW);
        Serial.write(incomingByte);
        break;
      case SERIAL_COMMAND_VOICE_LIGHTSON1:
        digitalWrite(PIN_LIGHTS_GROUP1, HIGH);
        Serial.write(incomingByte);
        break;
      case SERIAL_COMMAND_VOICE_LIGHTSOFF1:
        digitalWrite(PIN_LIGHTS_GROUP1, LOW);
        Serial.write(incomingByte);
        break;
      case SERIAL_COMMAND_VOICE_LIGHTSON2:
        digitalWrite(PIN_LIGHTS_GROUP2, HIGH);
        Serial.write(incomingByte);
        break;
      case SERIAL_COMMAND_VOICE_LIGHTSOFF2:
        digitalWrite(PIN_LIGHTS_GROUP2, LOW);
        Serial.write(incomingByte);
        break;
    }

/*
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
    
*/
    Serial.flush();
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
  }

}