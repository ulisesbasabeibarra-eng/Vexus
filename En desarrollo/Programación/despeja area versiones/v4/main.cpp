#include <Arduino.h>
#include <BluetoothSerial.h>

// pines para sharps
#define S1 39// sensor izq
#define S2 32 //sensor izq 45
#define S3 33 // sensor del medio 
#define S4 26 //sensor der 45
#define S5 27 // sensor der 

//pines para tcrt
#define T1 36 // tcrt de la derecha
#define T2 13 //tcrt de la izquierda
#define T3 5 // tcrt trasero

// ===== Motores ====
#define IN1A 22
#define IN1B 23 //bts 1
#define IN2A 4
#define IN2B 15 //bts 2

#define VMAX 80 //APROX  RPM
#define VBASE 70 //APROX  RPM
#define VMIN 60 //APROX  RPM

#define led 2
#define boton 17
#define DISTANCIA 1700 //para sensores - luego hacer pruebassss

unsigned long tiempo = 250;


bool esperando_inicio = false; 
bool activo = false;
unsigned long tiempo_ini = 0;
ulong prev_time, current_time, tiempo_trans, time_luz;
uint16_t tiempo_led = 3000,  tiempo_comp = 5000;


// PWM's
int PWM1 = VBASE;  //pwm de la izquierda
int PWM2 = VBASE;  //pwm de la derecha

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
    ATACAR,
    BUSCAR,
    ATRAS,
    BUSCAR_IZ,
    BUSCAR_IZQ45,
    BUSCAR_DE,
    BUSCAR_DE45
};
byte MODOS = BUSCAR;

float lectura(int pin){
  int suma = 0;
  for(int i = 0; i < 20; i++){
    suma += analogRead(pin);
  }
  return suma / 20;
}

int sensores (){
int suma_binaria = 0;

    if (lectura(S3) <= DISTANCIA){ // sensor del medio       // -
        suma_binaria += 4;                                   //  |
    }                                                        //  |    
    else if (lectura(S1) <= DISTANCIA){ // sensor izq        //  |        Prioriza los
        suma_binaria += 1;                                   //  |   
    }                                                        //  |      3 más importantes
    else if (lectura(S5) <= DISTANCIA){ // sensor der        //  |
        suma_binaria += 16;                                  //  |
    }                                                        // - 
    else if (lectura(S2) <= DISTANCIA){//sensor izq 45
        suma_binaria += 2;
    }
     else if (lectura(S4) <= DISTANCIA){//sensor der 45
        suma_binaria += 8;
    }
return suma_binaria;
}

void motores(int izq, int der) {
  if (izq >= 0) {
    if(izq > 255){ izq = 255; }
    ledcWrite(ledChannel, izq);
    ledcWrite(ledChannel1, 0);
  } else {
    izq = izq * (-1);
    if(izq > 255){ izq = 255; }
    ledcWrite(ledChannel, 0);
    ledcWrite(ledChannel1, izq);
  }
  
  if (der >= 0) {
    if(der > 255){ der = 255; }
    ledcWrite(ledChannel2, der);
    ledcWrite(ledChannel3, 0);
  } else {
    der = der * (-1);
    if(der > 255){ der = 255; }
    ledcWrite(ledChannel2, 0);
    ledcWrite(ledChannel3, der);
  }
}

void salir() { 
    if (!digitalRead(T1)) { 
        unsigned long tiempo_inicio = millis(); 
        while (millis() < tiempo_inicio + 400) {
            motores(-VMAX, -VMAX); 
        }
        MODOS = BUSCAR_IZ; 
    }
    else if (!digitalRead(T2)) { 
        unsigned long tiempo_inicio = millis(); 
        while (millis() < tiempo_inicio + 400) {
            motores(-VMAX, -VMAX); 
        }
        MODOS = BUSCAR_DE; 
    }
    else if (!digitalRead(T3)) { 
        unsigned long tiempo_inicio = millis(); 
        while (millis() < tiempo_inicio + 400) {
            motores(-VMAX, -VMAX); 
        }
        MODOS = BUSCAR_DE; 
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

while(digitalRead(boton)){
    Serial.println("Esperando boton...");
  }

/*
  digitalWrite(led, LOW);
  delay (3000);
  digitalWrite(led, HIGH);
  delay (2000);
*/
  while(digitalRead(boton)){}

}

void loop() {

    bool borde_der = !digitalRead(T1); // TRUE si detecta línea blanca
    bool borde_izq = !digitalRead(T2); // TRUE si detecta línea blanca
    bool borde_atra = !digitalRead(T3); //TRUE si detecta línea blanca

    if (lectura(S3) <= DISTANCIA){ // sensor del medio       // -
        MODOS = ATACAR;                                      //  |
    }                                                        //  |    
    else if (lectura(S1) <= DISTANCIA){ // sensor izq        //  |        Prioriza los
        MODOS = BUSCAR_IZ;                                   //  |   
    }                                                        //  |      3 más importantes
    else if (lectura(S5) <= DISTANCIA){ // sensor der        //  |
        MODOS = BUSCAR_DE;                                   //  |
    }                                                        // - 
    else if (lectura(S2) <= DISTANCIA){//sensor izq 45
        MODOS = BUSCAR_IZQ45;
    }
     else if (lectura(S4) <= DISTANCIA){//sensor der 45
        MODOS = BUSCAR_DE45;
    }


    if (borde_izq || borde_der){//tcrt izq y der
        MODOS = ATRAS;
        }
    else if (borde_atra){
        MODOS = ATACAR;
        }

    SerialBT.print(MODOS);

    switch (MODOS){
    case BUSCAR:
        motores(VMIN, VMAX);

        break;

    case BUSCAR_DE:
        motores(VMIN, VMAX);
       /* Serial.print("\nSensor derecho:");
        Serial.print(lectura(S5));
        delay (500);*/
        break;

    case BUSCAR_DE45:
        motores(VBASE, VMAX);
      /*  Serial.print("\nSensor dere45:");
        Serial.print(lectura(S4));
        delay (500);*/
        break;

    case BUSCAR_IZ:
        motores(VMAX, VMIN);
      /*  Serial.print("\nSensor izquierdo:");
        Serial.print(lectura(S1));
        delay (500);*/
        break;

    case BUSCAR_IZQ45:
        motores(VMAX, VBASE);
        /*Serial.print("\nSensor izq45:");
        Serial.print(lectura(S2));
        delay (500);*/
        break;

    case ATACAR:
        motores(VMAX, VMAX);
       /* Serial.print("\nSensor delantero:");
        Serial.print(lectura(S3));
        delay (500);*/
        break;

    case ATRAS:
        salir();
        break;

    default:
        break;
    }

}

