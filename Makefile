DEBUG=-g -DDEBUG
CC=g++ #$(DEBUG)
CFLAGS=-Wall -Wextra -O2 -march=core2 #$(DEBUG)

OBJECTS=MyBot.o Logger.o vec3.o PlanetWars.o Simulator.o Map.o KnapSack.o
VERSION=`git describe --tags`
TARGET=E323

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)-$(VERSION)

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf $(TARGET)* *.o *.txt

zip:
	zip $(TARGET)-$(VERSION).zip *.cc *.h
