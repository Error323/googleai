DEBUG=-g -rdynamic -DDEBUG
CC=g++ -O2 -march=core2 $(DEBUG)
CFLAGS=-Wall -Wextra $(DEBUG)

OBJECTS=MyBot.o AlphaBeta.o Timer.o Logger.o vec3.o PlanetWars.o Simulator.o Map.o KnapSack.o
VERSION=`git describe --tags`
TARGET=E323

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)-$(VERSION)

%.o: %.cc
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o *.txt

realclean: clean
	rm -rf $(TARGET)*

zip:
	zip $(TARGET)-$(VERSION).zip *.cc *.h *.inl
