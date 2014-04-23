TARGET  = random
LIBS    =
CC      = clang
CFLAGS  = -g -Wall
LDFLAGS =

OBJECTS = random.o ../random.o
HEADERS = $(wildcard ../*.h)

default: clean $(TARGET)
	./$(TARGET)

all: default

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f ../*.o
	-rm -f $(TARGET)

.PHONY: default all clean
.PRECIOUS: $(TARGET) $(OBJECTS)
