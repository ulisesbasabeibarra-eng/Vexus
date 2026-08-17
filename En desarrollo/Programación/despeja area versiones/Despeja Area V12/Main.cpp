#include <Arduino.h>
#include <BluetoothSerial.h>

// pines para sharps
#define S1 39 // sensor der
#define S2 34 // sensor der 45
#define S3 25 // sensor del medio 
#define S4 26 // sensor izq 45
#define S5 27 // sensor izq

//pines para tcrt
#define T1 36 // tcrt de la derecha
#define T2 13 //tcrt de la izquierda
#define T3 5 // tcrt trasero

// ===== Motores ====
#define IN1A 22
#define IN1B 23 //bts 1 motor izquierdo
#define IN2A 4
#define IN2B 15 //bts 2 motor derecho

#define VMAX_DER 110
#define VBASE_DER 100
#define VMIN_DER 90

#define VMAX_IZQ 130
#define VBASE_IZQ 120
#define VMIN_IZQ 110

#define led 2
#define boton 17
#define DISTANCIA 1700 //para sensores - luego hacer pruebassss

unsigned long tiempo = 250;

unsigned long tiempo_inicio_ataque = 0;
unsigned long tiempo_deteccion_lateral = 0;
unsigned long tiempo_inicio_retroceso = 0;
unsigned long duracion_retorno = 0;

int mejor_lectura = 0; // guarda el valor ADC más alto (más cercano)
int lado_memorizado = 0;   // 0 = ninguno, 1 = izquierda, 2 = derecha

bool objetivo_memorizado = false;

// PWM's
int PWM1 = VBASE_IZQ;  //pwm de la izquierda
int PWM2 = VBASE_DER;  //pwm de la derecha

// Configuración de PWM para control de motores
const int frequency = 1000;
const int resolution = 8;

// Canales PWM del ESP32
const int ledChannel = 0;
const int ledChannel1 = 1;
const int ledChannel2 = 2;
const int ledChannel3 = 3;

// ===== INICIALIZACION DEL BLUETOOTH =====
BluetoothSerial SerialBT;

// ===== MODOS =====
enum MODOS{
    SALIR_ADE,
    ATACAR,
    BUSCAR,
    ATRAS,
    BUSCAR_IZ,
    BUSCAR_IZQ45,
    BUSCAR_DE,
    BUSCAR_DE45,
    RETROCEDER_MEMORIA
};
byte MODOS = BUSCAR;

float lectura(int pin){
  int suma = 0;
  for(int i = 0; i < 20; i++){
    suma += analogRead(pin);
  }
  return suma / 20;
}

void motores(int izq, int der) {

  izq = constrain(izq, -250, 250); //250 para no forzar el motor al maximo
  der = constrain(der, -250, 250); //250 para no forzar el motor al maximo   

  if (izq >= 0) {
    ledcWrite(ledChannel, izq);
    ledcWrite(ledChannel1, 0);
  } else {
    izq = izq * (-1);
    ledcWrite(ledChannel, 0);
    ledcWrite(ledChannel1, izq);
  }
  
  if (der >= 0) {
    ledcWrite(ledChannel2, der);
    ledcWrite(ledChannel3, 0);
  } else {
    der = der * (-1);
    ledcWrite(ledChannel2, 0);
    ledcWrite(ledChannel3, der);
  }
}

void salir() { 
    if (!digitalRead(T1)) { 
        unsigned long tiempo_inicio = millis(); 
        while (millis() < tiempo_inicio + 900) {
            motores(-VMAX_IZQ, -VMAX_DER);
        }
//        MODOS = BUSCAR_IZ; 
    }
    else if (!digitalRead(T2)) { 
        unsigned long tiempo_inicio = millis(); 
        while (millis() < tiempo_inicio + 900) {
            motores(-VMAX_IZQ, -VMAX_DER);
        }
        MODOS = BUSCAR_DE; 
    }
}

void salir_adelante(){
     if (!digitalRead(T3)) { 
        unsigned long tiempo_inicio = millis(); 
        while (millis() < tiempo_inicio + 900) {
            motores(VMAX_IZQ, VMAX_DER);
        }
        MODOS = BUSCAR; 
    }
}

void setup() {
    Serial.begin(115200);
    SerialBT.begin("vexus");
/* declaracion de pines sharp, unicamente se menciona por el funcionamiento del adc del esp
 for (int i = S1; i <= S5; i++){
    pinMode(i, INPUT);
 }
*/

  // declaracion de pines tcrt  
  pinMode(T1, INPUT);
  pinMode(T2, INPUT);
  pinMode(T3, INPUT);

  // declaracion de pines motores
  pinMode(IN1A, OUTPUT);
  pinMode(IN1B, OUTPUT);
  pinMode(IN2A, OUTPUT);
  pinMode(IN2B, OUTPUT);

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  pinMode(boton, INPUT_PULLUP);

  // Configuración de PWM en cada canal y pin
  ledcSetup(ledChannel, frequency, resolution);
  ledcAttachPin(IN1A, ledChannel);

  ledcSetup(ledChannel1, frequency, resolution);
  ledcAttachPin(IN1B, ledChannel1);

  ledcSetup(ledChannel2, frequency, resolution);
  ledcAttachPin(IN2A, ledChannel2);

  ledcSetup(ledChannel3, frequency, resolution);
  ledcAttachPin(IN2B, ledChannel3);
  delay(100);

  motores(0,0);

while(digitalRead(boton)){}
  digitalWrite(led, LOW);
  delay (3000);
  digitalWrite(led, HIGH);
  delay (2000);
  digitalWrite(led, LOW);
}

void loop() {

   bool borde_der = !digitalRead(T1); // TRUE si detecta línea blanca
   bool borde_izq = !digitalRead(T2); // TRUE si detecta línea blanca
   bool borde_atra = !digitalRead(T3); //TRUE si detecta línea blanca

    if (lectura(S3) <= 2000){ // sensor del medio       
        MODOS = ATACAR;                                     
    }                                                          
     else if (lectura(S2) <= DISTANCIA){//sensor der 45
        MODOS = BUSCAR_DE45;
    }
     else if (lectura(S4) <= DISTANCIA){//sensor izq 45
        MODOS = BUSCAR_IZQ45;
    }
    else if (lectura(S1) <= DISTANCIA){ // sensor der       
        MODOS = BUSCAR_DE;                                    
    }                                                      
    else if (lectura(S5) <= DISTANCIA){ // sensor izq     
        MODOS = BUSCAR_IZ;                                
    }                                                         

    else {
        MODOS = BUSCAR;
    }

    if (borde_izq || borde_der){//tcrt izq y der
        MODOS = ATRAS;
        }
    else if (borde_atra){
        MODOS = SALIR_ADE;
        }

    SerialBT.print(MODOS);

    switch (MODOS){
    case BUSCAR:
        motores(VMIN_IZQ, VMAX_DER);

        break;

    case BUSCAR_DE:
        motores(VMAX_IZQ, VMIN_DER);

        break;

    case BUSCAR_DE45:
        motores(-VBASE_IZQ, VBASE_DER);

        break;
 
    case BUSCAR_IZ:
        motores(VMIN_IZQ, VMAX_DER);

        break;

    case BUSCAR_IZQ45:
        motores(VBASE_IZQ, -VBASE_IZQ);

        break;

    case ATACAR:
        motores(VMAX_IZQ, VMAX_DER);
        {
            int lectura_izq = lectura(S5); // Sensor izquierdo (90°)
            int lectura_der = lectura(S1); // Sensor derecho (90°)

            // Si detectamos algo a la izquierda y es el objeto más cercano hasta ahora
            if (lectura_izq > 1000 && lectura_izq > mejor_lectura) { 
                mejor_lectura = lectura_izq;
                tiempo_deteccion_lateral = millis();
                lado_memorizado = 1;
                objetivo_memorizado = true;
            }
            // Si detectamos algo a la derecha y es más cercano que lo memorizado
            else if (lectura_der > 1000 && lectura_der > mejor_lectura) {
                mejor_lectura = lectura_der;
                tiempo_deteccion_lateral = millis();
                lado_memorizado = 2;
                objetivo_memorizado = true;
            }
        }
        if (borde_der || borde_izq ) {
            if (objetivo_memorizado) {
                // calcula cuanto tiempo paso desde qeu vio el objeto hasta que toco el borde
                unsigned long tiempo_hasta_borde = millis() - tiempo_deteccion_lateral;
                
                // factor de corrección (ej. 0.8) porque retroceder sin empujar peso es más rápido que avanzar empujando
                float factor_correccion = 0.8; 
                duracion_retorno = tiempo_hasta_borde * factor_correccion;
                
                tiempo_inicio_retroceso = millis();
                MODOS = RETROCEDER_MEMORIA;
            } else {
                // Si no vio nada, simplemente aplica la rutina de salir normal
                MODOS = ATRAS;
            }
        }

    case ATRAS:
        salir();
        break;

    case SALIR_ADE:
        salir_adelante();
        break;

    case RETROCEDER_MEMORIA:
        motores(-VMAX_IZQ, -VMAX_DER); // Retrocedemos a la misma velocidad base

        // Si ya pasó el tiempo calculado para quedar alineados con el nuevo bloque
        if (millis() - tiempo_inicio_retroceso >= duracion_retorno) {
            motores(0, 0);
            
            // Reinicio de variables
            objetivo_memorizado = false;
            mejor_lectura = 0;

            // gira hacia el lado donde vio el objetivo
            if (lado_memorizado == 1) {
                MODOS = BUSCAR_IZ; // o un estado específico para girar 90° a la izquierda
            } else if (lado_memorizado == 2) {
                MODOS = BUSCAR_DE; // o un estado específico para girar 90° a la derecha
            } else {
                MODOS = BUSCAR;
            }
        }
        // la lógica general del loop() lo captura antes del switch y cambiará a SALIR_ADE (bode_atra)
        break;

    default:
        break;
    }

}
