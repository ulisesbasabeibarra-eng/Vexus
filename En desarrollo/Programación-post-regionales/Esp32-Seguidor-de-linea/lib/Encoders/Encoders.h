#ifndef ENCODERS_H
#define ENCODERS_H

#include <Arduino.h>
#include <Wire.h>

class Encoders{
private:


const int angulo = 0x0;

//encoder 1-primer canal I2C-
const int SDA1 = 8;
const int SCL1 = 9;
//encoder 2-segundo canal I2C-
const int SDA2 = 21;
const int SCL2 = 3;

//dirección de los encoders
const int as5600_direccion = 0x36;

// Variables de estado Encoder 1
int posicionAnterior1 = 0;
unsigned long ultimoTiempo1 = 0;
float rpmActual1 = 0;

// Variables de estado Encoder 2
int posicionAnterior2 = 0;
unsigned long ultimoTiempo2 = 0;
float rpmActual2 = 0;

//const int promRPM1;
//const int promRPM2;

// Lectura de ángulos por separado
int leerAngulo1();
int leerAngulo2();

public:
Encoders();
void begin();
// Cálculo de RPM por separado
float calculoRPM1();
float calculoRPM2();

};
#endif