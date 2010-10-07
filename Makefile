CC=g++
CFLAGS=-Wall -Wextra -O2
LDFLAGS=

OBJECTS=MyBot.o PlanetWars.o Simulator.o AlphaBeta.o
EXECUTABLE=MyBot

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf MyBot *.o *.txt
