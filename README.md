# PicoAtom
Simple Acorn Atom emulator running on the Raspbery Pi Pico.

Using an ILI9341 display, SD card and PS/2 keyboard.

Connections :

Pico pin | Purpose | Purpose | Pico pin
---------|---------|---------|--------
1 | Serial Tx for debug || 40
2 | Serial Rx for debug |5V output to PS/2 (and level shifter)| 39
3 | Serial GND | GND | 38
4 |	Display SCK || 37
5 |	Display MOSI | 3.3V output to Level shifter, Display and SD card | 36
6 |	Display MISO || 35
7 |	Display CS || 34
8 |	GND | GND | 33
9 |	Display CD (Command/Data) || 32
10 |	Display reset || 31
11 |	PS/2 Clock **NOTE** Via 5.0V <-> 3.3V level shifter || 30
12 |	PS/2 Data **NOTE** Via 5.0V <-> 3.3V level shifter || 29
13 |	GND | GND | 28
14 |	SD SCK || 27
15 |	SD MOSI || 26
16 |	SD MISO || 25
17 |	SD CS || 24
18 |	GND | GND | 23
19 |	Speaker out || 22
20 |	 || 21

All pins not explicitly specified are no connection.

It is suggested that a bidirectional level shifter is uded between the PS/2 keyboard and the Pico.
This allows the Pico to send commands to the keyboard such as toggling LEDs etc.
