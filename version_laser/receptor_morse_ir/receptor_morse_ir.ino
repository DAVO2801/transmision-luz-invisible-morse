/*
  RECEPTOR - Recepción y decodificación de código Morse por láser infrarrojo
  Proyecto: Transmisión de información mediante luz invisible (FSO, modulación OOK)
  Teoría de Señales - Universidad de Manizales

  USO
   - Abrir el Monitor Serie a 9600 baudios. El texto recibido aparece ahí.
   - Comandos (escribir la letra y presionar Enter):
       n -> mide el nivel de RUIDO (con el láser apagado)
       s -> mide el nivel de SEÑAL (con el láser fijo encendido: "#ON" en el emisor)
            Al tener ambas medidas se calcula la SNR y se ajusta el umbral analógico.
       e -> muestra estadísticas (caracteres recibidos, no reconocidos, ruido)
       r -> reinicia las estadísticas

  CONEXIONES
   Sensor:
     5V -> cátodo (K) del fotodiodo IR (queda en polarización inversa)
     ánodo (A) del fotodiodo -> nodo S -> resistencia 100 kΩ -> GND
     (con fototransistor: colector -> 5V, emisor -> nodo S -> 10 kΩ -> GND)
     nodo S -> A0 (para medir el nivel de señal y la SNR)
   Comparador LM393 (comparador A):
     pin 8 -> 5V, pin 4 -> GND, condensador de 100 nF entre pin 8 y GND
     pin 3 (IN+) -> nodo S
     pin 2 (IN-) -> cursor del potenciómetro de 10 kΩ (extremos a 5V y GND) = umbral
     pin 1 (OUT) -> D2, con resistencia pull-up de 10 kΩ a 5V
   D13 -> LED: se enciende cuando llega luz (ayuda a alinear el láser)
   D9  -> buzzer pasivo (opcional): suena con cada pulso recibido

  Con más luz, el nodo S sube de voltaje. Si supera el umbral del potenciómetro,
  la salida del LM393 queda en ALTO.
*/

const byte PIN_SENSOR = 2;     // salida del comparador
const byte PIN_ANALOG = A0;    // nodo S, sin pasar por el comparador
const byte PIN_LED    = 13;
const byte PIN_BUZZER = 9;

// Duración de un punto en milisegundos. DEBE ser igual en el emisor.
const unsigned int UNIDAD = 60;

// Cambios más cortos que esto se descartan como ruido (ms).
const unsigned int ANTIRREBOTE = 5;

// true: usa el LM393 en D2. false: compara A0 contra umbralAnalogico (sin comparador).
const bool USAR_COMPARADOR = true;
const bool NIVEL_LUZ = HIGH;   // valor de D2 cuando llega el láser

const bool USAR_BUZZER = true;
const bool MOSTRAR_TIEMPOS = false;  // true: imprime la duración de cada pulso (para el informe)

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

int umbralAnalogico = 300;     // 0-1023, se recalcula con los comandos n y s

// Símbolos de la letra en curso
char simbolos[8];
byte nSimbolos = 0;
bool desbordado = false;

// Estado de la señal (con antirrebote)
bool estadoEstable = false;
bool estadoCrudo = false;
unsigned long tUltimoCambio = 0;
unsigned long tCrudo = 0;

bool esperandoEspacio = false;
bool esperandoFin = false;

// Estadísticas
unsigned long nCaracteres = 0;
unsigned long nDesconocidos = 0;
unsigned long nRuido = 0;

// Medidas para la SNR (voltios, negativo = sin medir)
float vRuido = -1;
float vSenal = -1;

bool hayLuz() {
  if (USAR_COMPARADOR) return digitalRead(PIN_SENSOR) == NIVEL_LUZ;
  return analogRead(PIN_ANALOG) > umbralAnalogico;
}

void indicadores(bool luz) {
  digitalWrite(PIN_LED, luz ? HIGH : LOW);
  if (!USAR_BUZZER) return;
  if (luz) tone(PIN_BUZZER, 700);
  else noTone(PIN_BUZZER);
}

char decodificar() {
  if (desbordado) return '*';
  simbolos[nSimbolos] = '\0';
  for (byte i = 0; i < N_TABLA; i++) {
    if (strcmp(TABLA[i].codigo, simbolos) == 0) return TABLA[i].c;
  }
  return '*';                   // secuencia que no corresponde a ningún carácter
}

void finDePulso(unsigned long duracion) {
  // Umbral de decisión a mitad de camino entre punto (1 u) y raya (3 u)
  char simbolo = (duracion < 2UL * UNIDAD) ? '.' : '-';
  if (nSimbolos < 7) simbolos[nSimbolos++] = simbolo;
  else desbordado = true;

  if (MOSTRAR_TIEMPOS) {
    Serial.print(F("\n[pulso "));
    Serial.print(duracion);
    Serial.print(F(" ms = "));
    Serial.print(simbolo);
    Serial.println(']');
  }
}

void revisarSilencio(unsigned long silencio) {
  // Más de 2 u sin luz: terminó la letra (entre símbolos hay 1 u, entre letras 3 u)
  if (nSimbolos > 0 && silencio >= 2UL * UNIDAD) {
    char c = decodificar();
    Serial.print(c);
    nCaracteres++;
    if (c == '*') nDesconocidos++;
    nSimbolos = 0;
    desbordado = false;
    esperandoEspacio = true;
    esperandoFin = true;
  }
  // Más de 5 u: espacio entre palabras (7 u)
  if (esperandoEspacio && silencio >= 5UL * UNIDAD) {
    Serial.print(' ');
    esperandoEspacio = false;
  }
  // Silencio largo: fin del mensaje
  if (esperandoFin && silencio >= 20UL * UNIDAD) {
    Serial.println();
    esperandoFin = false;
  }
}

float medirVoltaje() {
  long suma = 0;
  for (int i = 0; i < 200; i++) {
    suma += analogRead(PIN_ANALOG);
    delayMicroseconds(500);
  }
  return (suma / 200.0) * 5.0 / 1023.0;
}

void mostrarSNR() {
  if (vRuido < 0 || vSenal < 0) return;
  // Un paso del ADC son ~4.9 mV; se usa como mínimo para no dividir por cero
  float ruido = max(vRuido, 0.0049);
  float snr = 20.0 * log10(vSenal / ruido);
  umbralAnalogico = (int)(((vSenal + vRuido) / 2.0) * 1023.0 / 5.0);
  Serial.print(F("SNR = "));
  Serial.print(snr, 1);
  Serial.print(F(" dB  |  umbral analogico ajustado a "));
  Serial.print(umbralAnalogico);
  Serial.print(F(" ("));
  Serial.print((vSenal + vRuido) / 2.0, 3);
  Serial.println(F(" V, use este voltaje en el potenciometro)"));
}

void mostrarEstadisticas() {
  Serial.println(F("\n--- Estadisticas ---"));
  Serial.print(F("Caracteres recibidos: "));
  Serial.println(nCaracteres);
  Serial.print(F("No reconocidos (*):   "));
  Serial.println(nDesconocidos);
  if (nCaracteres > 0) {
    Serial.print(F("Tasa de error:        "));
    Serial.print(100.0 * nDesconocidos / nCaracteres, 1);
    Serial.println(F(" %"));
  }
  Serial.print(F("Pulsos de ruido descartados: "));
  Serial.println(nRuido);
}

void atenderComandos() {
  char c = tolower(Serial.read());
  if (c == 'n') {
    vRuido = medirVoltaje();
    Serial.print(F("\nRuido (laser apagado): "));
    Serial.print(vRuido, 3);
    Serial.println(F(" V"));
    mostrarSNR();
  } else if (c == 's') {
    vSenal = medirVoltaje();
    Serial.print(F("\nSenal (laser encendido): "));
    Serial.print(vSenal, 3);
    Serial.println(F(" V"));
    mostrarSNR();
  } else if (c == 'e') {
    mostrarEstadisticas();
  } else if (c == 'r') {
    nCaracteres = nDesconocidos = nRuido = 0;
    Serial.println(F("\nEstadisticas reiniciadas"));
  }
}

void setup() {
  pinMode(PIN_SENSOR, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  Serial.begin(9600);
  Serial.println(F("=== RECEPTOR MORSE IR ==="));
  Serial.println(F("Comandos: n = ruido, s = senal, e = estadisticas, r = reiniciar"));
  Serial.println(F("Esperando mensajes...\n"));
}

void loop() {
  unsigned long ahora = millis();
  bool lectura = hayLuz();

  if (lectura != estadoCrudo) {
    if (lectura == estadoEstable) nRuido++;  // el cambio se revirtió antes de confirmarse
    estadoCrudo = lectura;
    tCrudo = ahora;
  }

  // Un cambio se acepta si se mantiene al menos ANTIRREBOTE ms
  if (estadoCrudo != estadoEstable && ahora - tCrudo >= ANTIRREBOTE) {
    unsigned long duracion = tCrudo - tUltimoCambio;
    estadoEstable = estadoCrudo;
    tUltimoCambio = tCrudo;
    indicadores(estadoEstable);

    if (estadoEstable) {
      // Volvió la luz: el mensaje continúa
      esperandoEspacio = false;
      esperandoFin = false;
    } else {
      finDePulso(duracion);
    }
  }

  if (!estadoEstable) revisarSilencio(ahora - tUltimoCambio);

  if (Serial.available()) atenderComandos();
}
