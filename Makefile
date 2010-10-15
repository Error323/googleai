CC=g++ -m32
CFLAGS=-Wall -Wextra -O2

OBJECTS=MyBot.o Logger.o PlanetWars.o Simulator.o vec3.o
TARGET=MyBot

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf MyBot *.o *.txt

zip:
	zip ${TARGET}-`git describe --tags`.zip *.cc *.h
