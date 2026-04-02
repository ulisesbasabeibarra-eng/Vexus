#include <Arduino.h>
#include <BluetoothSerial.h>

// pines para sharps
#define S1 14
#define S2 19
#define S3 18
#define S4 21
#define S5 16

//pines para tcrt
#define T1 36
#define T2 13
#define T3 5

// ===== Motores ====
#define IN1A 22
#define IN1B 23 //bts 1
#define IN2A 4
#define IN2B 15 //bts 2
#define VMAX 160 //APROX  RPM
#define VBASE 140 //APROX  RPM
#define VMIN 120 //APROX  RPM

#define led 2
#define boton 17
#define DISTANCIA 2000 //para sensores - luego hacer pruebassss

unsigned long tiempo = 250;
unsigned long tiempo_inicio_espera = 0; 
unsigned long tiempo_inicio_retroceso = 0;

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
byte MODOS = BUSCAR;
enum MODOS{
    ATACAR,
    BUSCAR,
    ATRAS,
    BUSCAR_IZ,
    BUSCAR_IZQ45,
    BUSCAR_DE,
    BUSCAR_DE45
};

void motores(int izq, int der) {  //0 hasta 255 adelante 0 hasta -255 atras

  if (izq >= 0) {
    ledcWrite(ledChannel, izq);
    ledcWrite(ledChannel1, 0);  //analog
  } else {
    izq = izq * (-1);
    ledcWrite(ledChannel1, izq);
    ledcWrite(ledChannel, 1);
  }
  //motor derecho//
  if (der >= 0) {
    ledcWrite(ledChannel2, der);
    ledcWrite(ledChannel3, 0);
  } else {
    der = der * (-1);
    ledcWrite(ledChannel3, der);
    ledcWrite(ledChannel2, 1);
  }
}

void prenderLedPorTiempo(unsigned long tiempo_ms){
    digitalWrite(led, LOW);
    delay(tiempo_ms);
    digitalWrite(led, HIGH);
}

void precaucion(unsigned long espera) {
motores(0,0);
prenderLedPorTiempo(espera);
}

void salir(){  

tiempo >= millis(); 

    if (!digitalRead(T1)){ // Evitar que se salga del tatami
        motores(-VMIN, -VMIN);
        millis();
        MODOS = BUSCAR_IZ;
        SerialBT.print("sensor izq tcrt:");
        SerialBT.println(digitalRead(T1));
    }
    else if (!digitalRead(T2)){
        motores(-VMIN, -VMIN);
        millis();
        MODOS = BUSCAR_DE;
        SerialBT.print("sensor der tcrt:");
        SerialBT.println(digitalRead(T2));
    }
    else if (!digitalRead(T3)){
        motores(-VMIN, -VMIN);
        millis();
        MODOS = BUSCAR_DE;
        SerialBT.print("sensor der tcrt:");
        SerialBT.println(digitalRead(T3));
    }
}

void setup() {
    Serial.begin(115200);

   // declaracion de pines sharp
 for (int i = S1; i <= S5; i++){
    pinMode(i, INPUT);
 }

  // declaracion de pines tcrt  
  pinMode(T1, INPUT);
  pinMode(T2, INPUT);
  pinMode(T3, INPUT);

  pinMode(IN1A, OUTPUT);
  pinMode(IN1B, OUTPUT);
  pinMode(IN2A, OUTPUT);
  pinMode(IN2B, OUTPUT);

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  pinMode(boton, INPUT_PULLUP);

  // Configuración de PWM en cada canal y pin

  //ledcAttach(IN1A, frequency, resolution);

  ledcSetup(ledChannel, frequency, resolution);
  ledcAttachPin(IN1A, ledChannel);

  ledcSetup(ledChannel1, frequency, resolution);
  ledcAttachPin(IN1B, ledChannel1);

  ledcSetup(ledChannel2, frequency, resolution);
  ledcAttachPin(IN2A, ledChannel2);

  ledcSetup(ledChannel3, frequency, resolution);
  ledcAttachPin(IN2B, ledChannel3);

  precaucion(5000); 
}

void loop() {

//if (digitalRead(boton)==0){}


    bool borde_atra = !digitalRead(T1); //TRUE si detecta línea blanca
    bool borde_izq = !digitalRead(T2); // TRUE si detecta línea blanca
    bool borde_der = !digitalRead(T3); // TRUE si detecta línea blanca
   
if (analogRead(S3) >= DISTANCIA){ // sensor del medio        // -
        MODOS = ATACAR;                                      //  |
    }                                                        //  |    
    else if (analogRead(S1) >= DISTANCIA){ // sensor izq     //  |        Prioriza los
        MODOS = BUSCAR_IZ;                                   //  |   
    }                                                        //  |      3 más importantes
    else if (analogRead(S5) >= DISTANCIA){ // sensor der     //  |
        MODOS = BUSCAR_DE;                                   //  |
    }                                                        // - 

    else if (analogRead(S2) >= DISTANCIA){//sensor izq 45
        MODOS = BUSCAR_IZQ45;
    }
     else if (analogRead(S4) >= DISTANCIA){//sensor der 45
        MODOS = BUSCAR_DE45;
    }

    if (borde_izq || borde_der){//tcrt izq y der
        MODOS = ATRAS;
    }
    else if (borde_atra){
        MODOS = ATACAR;
    }

    switch (MODOS){
    case BUSCAR:
        motores(VMIN, VMAX);

        break;

    case BUSCAR_DE:
        motores(VMIN, VMAX);

        break;

    case BUSCAR_DE45:
        motores(VBASE, VMAX);

        break;

    case BUSCAR_IZ:
        motores(VMAX, VMIN);

        break;

    case BUSCAR_IZQ45:
        motores(VMAX, VBASE);

        break;

    case ATACAR:
        motores(VMAX, VMAX);

        break;

    case ATRAS:
        salir();
        //motores(-VMIN,-VMIN);
        break;

    default:
        break;
    }

  }  

