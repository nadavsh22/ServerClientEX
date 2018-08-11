.DEFAULT_goal := all

CC = g++
CFLAGS = -std=c++11 -Wall -g -DNDEBUG

.PHONT: all clean

# make all files
all: whatsappServer whatsappClient

# make client
whatsappClient: whatsappClient.o whatsappio.o whatsappio.h Protocol.o Protocol.h
	$(CC) $(CFLAGS) whatsappClient.o whatsappio.o Protocol.o -o whatsappClient

whatsappClient.o: whatsappClient.cpp whatsappClient.h whatsappio.cpp whatsappio.h
	$(CC) $(CFLAGS) whatsappClient.cpp -c

# make server
whatsappServer: whatsappServer.o whatsappio.o whatsappio.h Protocol.o Protocol.h
	$(CC) $(CFLAGS) whatsappServer.o whatsappio.o Protocol.o -o whatsappServer

whatsappServer.o: whatsappServer.cpp whatsappServer.h whatsappio.cpp whatsappio.h
	$(CC) $(CFLAGS) whatsappServer.cpp -c


# compile helpers
whatsappio.o: whatsappio.cpp whatsappio.h
	$(CC) $(CFLAGS) whatsappio.cpp -c

Protocol.o: Protocol.cpp Protocol.h
	$(CC) $(CFLAGS) Protocol.cpp -c	

# make tar
tar: whatsappServer whatsappClient README
	tar -cvf ex4.tar Protocol.cpp Protocol.h whatsappServer.cpp whatsappClient.cpp whatsappServer.h whatsappClient.h whatsappio.h whatsappio.cpp Makefile README 

# clean directory
clean:
	rm -f *.o ex4.tar whatsappServer whatsappClient  
