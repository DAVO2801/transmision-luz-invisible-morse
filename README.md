# Transmisión de información mediante luz invisible (Morse por láser IR)

Proyecto de **Teoría de Señales** — Universidad de Manizales.

Prototipo de comunicación óptica en espacio libre (**FSO**) que transmite texto codificado en **código Morse** usando un **diodo láser infrarrojo** (luz invisible) y modulación **OOK** (láser encendido = pulso, apagado = espacio). Un segundo Arduino recibe la luz con un fotodiodo, decodifica el Morse y muestra el texto.

**Integrantes:** Juan Felipe Orozco · Daniel Villegas Toro · Juan Miguel Gómez
**Docente:** Mateo Cardona Marín

## Objetivo

Diseñar, implementar y evaluar un prototipo de transmisión y recepción de datos mediante láser infrarrojo, midiendo:

- precisión en la demodulación del código Morse,
- tiempo de respuesta del sensor,
- alcance frente a la alineación del haz,
- efecto del ruido óptico ambiental (relación señal/ruido).

## Estructura

```
emisor_morse_ir/      Sketch del emisor (texto -> Morse -> láser IR)
receptor_morse_ir/    Sketch del receptor (fotodiodo -> Morse -> texto, SNR, estadísticas)
docs/                 Marco teórico (Word)
```

## Funcionamiento

```
[Monitor Serie] -> Arduino emisor -> transistor -> láser IR 850 nm
                                                     |  (haz invisible)
                                                     v
[Monitor Serie] <- Arduino receptor <- LM393 <- fotodiodo PIN + filtro IR
```

Tiempos Morse estándar, con `UNIDAD = 60 ms` (~20 palabras por minuto):

| Elemento | Duración |
|---|---|
| Punto | 1 u |
| Raya | 3 u |
| Entre símbolos | 1 u |
| Entre letras | 3 u |
| Entre palabras | 7 u |

El receptor clasifica cada pulso como punto (< 2 u) o raya (≥ 2 u) y usa los silencios para separar letras (≥ 2 u) y palabras (≥ 5 u). `UNIDAD` debe ser igual en ambos sketches.

### Emisor
- Escribir un mensaje en el Monitor Serie (9600 baudios, "Nueva línea") y presionar Enter.
- `#ON` / `#OFF`: láser encendido fijo, para alinear y medir la señal.
- Pulsador opcional en D2 como manipulador Morse manual.

### Receptor
- Muestra el texto recibido en el Monitor Serie.
- `n`: mide el ruido (láser apagado) · `s`: mide la señal (láser con `#ON`) → calcula la **SNR en dB** y sugiere el umbral del potenciómetro.
- `e`: estadísticas (caracteres, no reconocidos, tasa de error, pulsos de ruido) · `r`: reiniciar.
- `MOSTRAR_TIEMPOS = true` imprime la duración de cada pulso.
- `USAR_COMPARADOR = false` permite funcionar sin el LM393 leyendo A0 directamente.

## Conexiones

**Emisor**
- D8 → 1 kΩ → base del NPN (2N2222/BC547); colector → GND del módulo láser; emisor → GND
- VCC del módulo láser → 5V
- D13 → LED indicador · D2 → pulsador → GND (opcional)

**Receptor**
- 5V → cátodo del fotodiodo; ánodo → nodo S → 100 kΩ → GND (fototransistor: colector a 5V, emisor a nodo S, 10 kΩ a GND)
- Nodo S → A0 y → LM393 pin 3 (IN+)
- Potenciómetro 10 kΩ (5V–GND), cursor → LM393 pin 2 (IN−)
- LM393 pin 1 (OUT) → D2 con pull-up de 10 kΩ; pin 8 → 5V, pin 4 → GND, 100 nF entre 8 y GND
- D13 → LED de alineación · D9 → buzzer pasivo (opcional)

## Materiales

| Emisor | Receptor |
|---|---|
| Arduino Uno/Nano | Arduino Uno/Nano |
| Módulo láser IR 850 nm, 5 V, ≤ 5 mW, con driver | Fotodiodo PIN IR (SFH 213 FA, BPV10NF o BPW34) o fototransistor IR (L14G1) |
| Transistor NPN 2N2222 / BC547 + 1 kΩ | Comparador LM393 + potenciómetro 10 kΩ |
| LED + 220 Ω | Resistencias 100 kΩ / 10 kΩ, condensadores 100 nF |
| Pulsador (opcional) | Filtro óptico IR pasa-largo (720–850 nm) |
| | LED + 220 Ω, buzzer pasivo, lente convexa y tubo negro (opcionales) |

Además: protoboards, jumpers, power bank, multímetro, cinta métrica y soportes para alinear.

> No usar receptores TSOP1838 (de control remoto): solo detectan luz modulada a 38 kHz.

## Seguridad

El haz infrarrojo **no es visible** y el ojo no tiene reflejo de parpadeo ante él. Usar potencias ≤ 5 mW, nunca apuntar a personas ni mirar el módulo de frente y, de ser posible, usar gafas de protección para 850 nm. Para alinear, la cámara frontal de muchos celulares muestra el punto IR.
