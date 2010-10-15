DEBUG=-g -DDEBUG
CC=g++ -m32 $(DEBUG)
CFLAGS=-Wall -Wextra -O2 $(DEBUG)

OBJECTS=MyBot.o Logger.o PlanetWars.o Simulator.o vec3.o
TARGET=MyBot

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf MyBot *.o *.txt

zip:
	zip $(TARGET)-`git describe --tags`.zip *.cc *.h
