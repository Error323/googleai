CC=g++ -O2

all: MyBot

clean:
	rm -rf *.o MyBot *.txt

MyBot: MyBot.o PlanetWars.o
