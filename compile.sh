avr-gcc -std=c99 -DF_CPU=9600000 -s -Os -mmcu=attiny13a -o main main.c
avr-objcopy -O ihex main main.hex
#avrdude -p t13 -c avrusb500 -eU flash:w:amp.hex
