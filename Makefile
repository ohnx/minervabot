INCLUDES=-Iinclude/
CFLAGS=$(INCLUDES) -Wall -Werror

default: dirs bin/minervabot

OBJ=netabs.o main.o
MODULESOBJ=simple.o

dirs:
	mkdir -p bin/
	mkdir -p bin/modules

%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(EXTRA)

bin/minervabot: $(OBJ)
	$(CC) -o $@ $(OBJ)

run:
	bin/minervabot

clean:
	rm $(OBJ)
	rm bin/minervabot

modules: $()
	??? something
