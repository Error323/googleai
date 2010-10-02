CC=g++ -O2 -Wall -Wextra

all: MyBot

clean:
	rm -rf MyBot *.o *.txt

MyBot: MyBot.o PlanetWars.o Simulator.o
