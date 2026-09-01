#ifndef OLED_H
#define OLED_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Oled {
private:
    const uint8_t boton1 = 43;
    const uint8_t boton2 = 44;
    const uint8_t boton3 = 42;
    const uint8_t led = 38;

    static const unsigned char vexusImagen[];


    void manejarCronometro();

public:
    Adafruit_SSD1306 display;

    Oled(); // Constructor
    void begin();
    void mostrarMensaje(String texto);

};
#endif