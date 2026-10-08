# Transmisión de información mediante luz invisible (Morse con control IR)

Proyecto de **Teoría de Señales** — Universidad de Manizales.

Un **control remoto infrarrojo** emite luz invisible al ojo humano. Cada pulsación se interpreta como un símbolo del **código Morse** —corta = punto, larga = raya—, un **Arduino** con un receptor IR la decodifica y el mensaje aparece en una **pantalla LCD 16x2**.

**Integrantes:** Juan Felipe Orozco · Daniel Villegas Toro · Juan Miguel Gómez
**Docente:** Mateo Cardona Marín

## Objetivo

Diseñar, implementar y evaluar un prototipo de transmisión de información mediante luz infrarroja, midiendo:

- precisión en la decodificación del código Morse,
- tiempo de respuesta del sistema,
- alcance y ángulo de recepción,
- efecto de la luz ambiental.

## Estructura

```
morse_ir_lcd/      Sketch principal: control IR -> receptor VS1838B -> Arduino -> LCD
version_laser/     Versión alternativa con láser IR y fotodiodo (dos Arduinos)
docs/              Marco teórico (Word)
```

## Funcionamiento

```
Control remoto IR  ──(luz IR 940 nm, portadora 38 kHz)──>  VS1838B  ->  Arduino  ->  LCD 16x2
 pulsación corta = .                                        demodula    decodifica   "HOLA"
 pulsación larga = -
```

El control no emite luz continua: modula su LED a **38 kHz** y el receptor solo responde a esa frecuencia (filtro pasabanda + demodulador), lo que lo hace inmune a la luz ambiental. Mientras el botón se mantiene presionado, el control repite su señal cada ~110 ms; el Arduino considera el botón soltado tras 160 ms sin señal.

| Parámetro (en el código) | Valor | Significado |
|---|---|---|
| `UMBRAL_RAYA` | 300 ms | Pulsación más corta = punto, más larga = raya |
| `PAUSA_LETRA` | 800 ms | Silencio que termina una letra |
| `PAUSA_PALABRA` | 2500 ms | Silencio que agrega un espacio (0 = desactivado) |
| `TIEMPO_SOLTAR` | 160 ms | Tiempo sin señal para considerar el botón soltado |

**Pantalla:** la fila 1 muestra el mensaje (últimos 16 caracteres) y la fila 2 los símbolos en curso y la letra que formarían (`.-.     >R`).

**Botones de función** (valores del control de los kits de Arduino; cualquier otro botón es la tecla Morse):

| Botón | Código | Acción |
|---|---|---|
| `>\|\|` | `0x43` | Espacio |
| `\|<<` | `0x44` | Borrar símbolos en curso o el último carácter |
| `CH-` | `0x45` | Limpiar pantalla |

Si el control es otro, abrir el Monitor Serie (9600 baudios): cada botón imprime su protocolo y código, y se cambian las constantes `BOTON_*` en el sketch. El Monitor Serie también muestra la duración de cada pulsación, útil para el informe.

## Conexiones

| Componente | Pin | Arduino |
|---|---|---|
| Receptor IR (VS1838B / TSOP1838 / KY-022) | OUT · VCC · GND | D2 · 5V · GND |
| LCD 16x2 con I2C | SDA · SCL · VCC · GND | A4 · A5 · 5V · GND |
| Buzzer **activo** (opcional) | + · − | D8 · GND |

El LED de la placa (D13) parpadea con cada señal IR. No usar buzzer pasivo con `tone()`: la librería IRremote usa el mismo temporizador.

## Materiales

- Arduino Uno + cable USB
- Control remoto infrarrojo (protocolo NEC, como el de los kits de Arduino)
- Receptor IR VS1838B / TSOP1838 o módulo KY-022
- Pantalla LCD 16x2 con módulo I2C (dirección 0x27 o 0x3F)
- Protoboard y cables jumper
- Buzzer activo y LED (opcionales)

## Software

Arduino IDE 2 con las librerías (Herramientas → Gestionar librerías):

- **IRremote** (Armin Joachimsmeyer), versión 4.x
- **LiquidCrystal I2C** (Frank de Brabander)

Si la LCD enciende pero no muestra texto: ajustar el contraste con el potenciómetro azul del módulo I2C o cambiar `DIRECCION_LCD` a `0x3F`.

## Versión con láser

La carpeta [`version_laser/`](version_laser) contiene la propuesta original: un Arduino emisor con diodo láser IR de 850 nm y un Arduino receptor con fotodiodo PIN, comparador LM393 y medición de SNR. Se conserva como alternativa para pruebas de mayor alcance.
