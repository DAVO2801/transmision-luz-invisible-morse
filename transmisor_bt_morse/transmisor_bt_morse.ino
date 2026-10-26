/*
  TRANSMISOR - Texto por Bluetooth -> código Morse por luz infrarroja
  Proyecto: Transmisión de información mediante luz invisible
  Teoría de Señales - Universidad de Manizales

  QUÉ HACE
   - Recibe texto desde el celular por el módulo Bluetooth HC-05.
   - Convierte cada letra a código Morse.
   - Enciende y apaga un LED infrarrojo modulado a 38 kHz, imitando pulsaciones
     cortas (punto) y largas (raya), para que el RECEPTOR que ya tienes
     (morse_ir_lcd, con el VS1838B y la LCD) lo decodifique y lo muestre.

  CÓMO PROBARLO
   1. Empareja el HC-05 en el celular (Ajustes -> Bluetooth, PIN 1234 o 0000).
   2. Abre la app "Serial Bluetooth Terminal", conéctate al HC-05.
   3. Escribe (o dicta con el micrófono del teclado) un mensaje y envíalo.
      El receptor debe mostrarlo en su LCD.
   También puedes escribir en el Monitor Serie (9600 baudios) para probar sin
   el celular.

  CONEXIONES
   HC-05:
     VCC -> 5V      GND -> GND
     TXD -> D10     (directo)
     RXD -> divisor de voltaje desde D11 (2 kΩ en serie + 1 kΩ a GND)
   LED infrarrojo con transistor:
     D3 -> 1 kΩ -> base del 2N2222
     colector -> cátodo (pata corta) del LED IR
     ánodo (pata larga) del LED IR -> 100 Ω -> 5V
     emisor -> GND

  LIBRERÍAS (Arduino IDE -> Gestor de librerías)
   - IRremote (Armin Joachimsmeyer), versión 4.x   [ya instalada]
   - SoftwareSerial viene incluida con el Arduino IDE

  IMPORTANTE: los tiempos de abajo están calculados para coincidir con el
  receptor morse_ir_lcd (UMBRAL_RAYA=300, PAUSA_LETRA=800, PAUSA_PALABRA=2500).
  Si cambias esos valores en el receptor, cámbialos también aquí.
*/

#include <IRremote.hpp>
#include <SoftwareSerial.h>

const byte PIN_IR_LED = 3;    // salida al LED infrarrojo (lo exige IRremote en el Uno/Nano)
const byte PIN_BT_RX  = 10;   // Arduino RX  <- TXD del HC-05
const byte PIN_BT_TX  = 11;   // Arduino TX  -> divisor -> RXD del HC-05

SoftwareSerial bt(PIN_BT_RX, PIN_BT_TX);

// Duraciones en milisegundos (deben ser coherentes con el receptor)
const unsigned int T_PUNTO   = 150;   // pulso corto  -> el receptor lo ve como punto (<300)
const unsigned int T_RAYA    = 500;   // pulso largo  -> el receptor lo ve como raya (>=300)
const unsigned int T_SIMBOLO = 350;   // silencio entre símbolos de una letra (>160 y <800)
const unsigned int T_LETRA   = 1100;  // silencio al terminar una letra (>800 y <2500)
const unsigned int T_PALABRA = 2900;  // silencio total para que inserte un espacio (>2500)

// Código NEC que se "mantiene presionado". Cualquiera que NO sea un botón de
// función del receptor (espacio 0x43, borrar 0x44, limpiar 0x45, corazón 0x47).
const uint8_t NEC_ADDR = 0x00;
const uint8_t NEC_CMD  = 0x15;

struct Morse { char c; const char* codigo; };

const Morse TABLA[] = {
  {'A', ".-"},     {'B', "-..."},   {'C', "-.-."},   {'D', "-.."},    {'E', "."},
  {'F', "..-."},   {'G', "--."},    {'H', "...."},   {'I', ".."},     {'J', ".---"},
  {'K', "-.-"},    {'L', ".-.."},   {'M', "--"},     {'N', "-."},     {'O', "---"},
  {'P', ".--."},   {'Q', "--.-"},   {'R', ".-."},    {'S', "..."},    {'T', "-"},
  {'U', "..-"},    {'V', "...-"},   {'W', ".--"},    {'X', "-..-"},   {'Y', "-.--"},
  {'Z', "--.."},
  {'0', "-----"},  {'1', ".----"},  {'2', "..---"},  {'3', "...--"},  {'4', "....-"},
  {'5', "....."},  {'6', "-...."},  {'7', "--..."},  {'8', "---.."},  {'9', "----."},
  {'.', ".-.-.-"}, {',', "--..--"}, {'?', "..--.."}, {'!', "-.-.--"},
  {'/', "-..-."},  {'=', "-...-"},  {'-', "-....-"}, {'@', ".--.-."}
};
const byte N_TABLA = sizeof(TABLA) / sizeof(TABLA[0]);

const byte MAX_LINEA = 64;
char linea[MAX_LINEA + 1];
byte nLinea = 0;

// Convierte a mayúscula y reemplaza tildes y ñ por su letra base (Morse no las tiene)
char normaliza(char c) {
  switch ((unsigned char)c) {
    case 0xE1: case 0xC1: return 'A';  // á Á
    case 0xE9: case 0xC9: return 'E';  // é É
    case 0xED: case 0xCD: return 'I';  // í Í
    case 0xF3: case 0xD3: return 'O';  // ó Ó
    case 0xFA: case 0xDA: return 'U';  // ú Ú
    case 0xF1: case 0xD1: return 'N';  // ñ Ñ
  }
  if (c >= 'a' && c <= 'z') return c - 32;
  return c;
}

const char* buscarCodigo(char c) {
  for (byte i = 0; i < N_TABLA; i++) {
    if (TABLA[i].c == c) return TABLA[i].codigo;
  }
  return nullptr;
}

// Mantiene la "pulsación" enviando tramas NEC repetidas durante 'ms' milisegundos
void enviarPulso(unsigned int ms) {
  unsigned long inicio = millis();
  do {
    IrSender.sendNEC(NEC_ADDR, NEC_CMD, 0);
  } while (millis() - inicio < ms);
}

void enviarCodigo(const char* codigo) {
  for (byte i = 0; codigo[i] != '\0'; i++) {
    enviarPulso(codigo[i] == '.' ? T_PUNTO : T_RAYA);
    if (codigo[i + 1] != '\0') delay(T_SIMBOLO);  // silencio solo entre símbolos
  }
}

void transmitir(const char* texto) {
  Serial.print(F(">> Transmitiendo: "));
  Serial.println(texto);
  for (byte i = 0; texto[i] != '\0'; i++) {
    char c = normaliza(texto[i]);
    if (c == ' ') {
      delay(T_PALABRA - T_LETRA);   // completa el silencio para un espacio
      continue;
    }
    const char* codigo = buscarCodigo(c);
    if (codigo == nullptr) continue;  // carácter sin equivalente en Morse: se omite
    Serial.print(F("   "));
    Serial.print(c);
    Serial.print(F(" "));
    Serial.println(codigo);
    enviarCodigo(codigo);
    delay(T_LETRA);                 // fin de letra
  }
  Serial.println(F(">> Listo"));
}

void procesarCaracter(char c) {
  if (c == '\n' || c == '\r') {
    if (nLinea > 0) {
      linea[nLinea] = '\0';
      transmitir(linea);
      nLinea = 0;
    }
  } else if (nLinea < MAX_LINEA) {
    linea[nLinea++] = c;
  }
}

void setup() {
  Serial.begin(9600);
  bt.begin(9600);                   // baudios por defecto del HC-05 en modo normal
  IrSender.begin(PIN_IR_LED);

  Serial.println(F("=== TRANSMISOR BT -> MORSE IR ==="));
  Serial.println(F("Envie texto por Bluetooth o escribalo aqui y presione Enter."));
}

void loop() {
  while (bt.available())     procesarCaracter(bt.read());
  while (Serial.available()) procesarCaracter(Serial.read());
}
