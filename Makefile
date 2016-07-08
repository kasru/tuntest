CXX=g++
CC=gcc
CFLAGS=-c -Wall
LDFLAGS=

SOURCES1=tun.c
OBJECTS1=$(SOURCES1:.c=.o)
EXEC1=tuntest

SOURCES2=tunctrl.c
OBJECTS2=$(SOURCES2:.c=.o)
EXEC2=tunctrl

all: $(SOURCES1) $(SOURCES2) $(EXEC1) $(EXEC2)

$(EXEC1): $(OBJECTS1)
	$(CC) $(LDFLAGS) $(OBJECTS1) -o $@

$(EXEC2): $(OBJECTS2)
	$(CC) $(LDFLAGS) $(OBJECTS2) -o $@

clean:
	rm -f $(OBJECTS1) $(EXEC1)
	rm -f $(OBJECTS2) $(EXEC2)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

.cpp.o:
	$(CXX) $(CFLAGS) $< -o $@

