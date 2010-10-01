CC=g++ -O2

all: MyBot

clean:
	rm -rf *.o MyBot

MyBot: MyBot.o PlanetWars.o
