CC=g++ -m32
CFLAGS=-Wall -Wextra -O2
LDFLAGS=

OBJECTS=MyBot.o Logger.o PlanetWars.o Simulator.o vec3.o
EXECUTABLE=MyBot

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf MyBot *.o *.txt
