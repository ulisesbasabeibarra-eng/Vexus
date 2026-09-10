#include "Encoders.h"

void Encoders::begin(){
 //----ENCODERS----
Wire.begin(SDA1,SCL1);
Wire1.begin(SDA2,SCL2); 

posicionAnterior1 = leerAngulo1();
posicionAnterior2 = leerAngulo2();

ultimoTiempo1 = millis();
ultimoTiempo2 = millis();
};

int Encoders::leerAngulo1() {
  Wire.beginTransmission(as5600_direccion);
  Wire.write(0x0C); // registro angulo
  Wire.endTransmission(false);
  Wire.requestFrom(as5600_direccion, 2);

  if (Wire.available() == 2) {
    int highByte = Wire.read();
    int lowByte = Wire.read();
    return ((highByte << 8) | lowByte) & 0x0FFF;
  }
  return -1;
}

int Encoders::leerAngulo2() {
  Wire1.beginTransmission(as5600_direccion);
  Wire1.write(0x0C); // Registro angulo
  Wire1.endTransmission(false);
  Wire1.requestFrom(as5600_direccion, 2);

  if (Wire1.available() == 2) {
    int highByte = Wire1.read();
    int lowByte = Wire1.read();
    return ((highByte << 8) | lowByte) & 0x0FFF;
  }
  return -1;
}

float Encoders::calculoRPM1() {
  unsigned long tiempoActual = millis();
  unsigned long deltaTime = tiempoActual - ultimoTiempo1;

  if (deltaTime >= 50) {
    int posicionActual = leerAngulo1();

    if (posicionActual != -1) {
      int diferenciaPosicion = posicionActual - posicionAnterior1;

      // Cruce de límite 0 - 4095
      if (diferenciaPosicion > 2048) {
        diferenciaPosicion -= 4096;
      } else if (diferenciaPosicion < -2048) {
        diferenciaPosicion += 4096;
      }

      float revoluciones = (float)diferenciaPosicion / 4096.0;
      float minutos = (float)deltaTime / 60000.0;
      rpmActual1 = revoluciones / minutos;

      posicionAnterior1 = posicionActual;
      ultimoTiempo1 = tiempoActual;
    }
  }
  return rpmActual1;
}

float Encoders::calculoRPM2() {
  unsigned long tiempoActual = millis();
  unsigned long deltaTime = tiempoActual - ultimoTiempo2;

  if (deltaTime >= 50) {
    int posicionActual = leerAngulo2();

    if (posicionActual != -1) {
      int diferenciaPosicion = posicionActual - posicionAnterior2;

      // Cruce de límite 0 - 4095
      if (diferenciaPosicion > 2048) {
        diferenciaPosicion -= 4096;
      } else if (diferenciaPosicion < -2048) {
        diferenciaPosicion += 4096;
      }

      float revoluciones = (float)diferenciaPosicion / 4096.0;
      float minutos = (float)deltaTime / 60000.0;
      rpmActual2 = revoluciones / minutos;

      posicionAnterior2 = posicionActual;
      ultimoTiempo2 = tiempoActual;
    }
  }
  return rpmActual2;
}