CC=gcc
CFLAGS=-O3 -std=c99

mcc-cshd-map-patcher:
	$(CC) main.c -o mcc-cshd-map-patcher

clean:
	rm -f mcc-cshd-map-patcher