#ifdef MOTORES_H
#define MOTORES_H

#include <Arduino.h>

class Motores{
private:
//----Motor izquierdo----
uint8_t IN1A;
uint8_t IN1B;
//----Motor derecho----
uint8_t IN2A;
uint8_t IN2B;

int frequency = 1000;
int resolution = 8;


}
#endif