INCLUDES=-Iinclude/
CFLAGS=$(INCLUDES) -Wall -Werror

default: dirs bin/minervabot
all: default modules

OBJ=obj/netabs.o obj/main.o
MODULESOBJ=bin/modules/simple.so

.PHONY: dirs
dirs:
	@mkdir -p bin/
	@mkdir -p bin/modules
	@mkdir -p obj/
	@mkdir -p obj/modules

obj/%.o: src/%.c
	$(CC) -c -o $@ $< $(CFLAGS) $(EXTRA)

bin/minervabot: $(OBJ)
	$(CC) -o $@ $(OBJ)

.PHONY: run
run:
	bin/minervabot

.PHONY: clean
clean:
	rm $(OBJ)
	rm bin/minervabot

obj/modules/%.o: modules/%.c
	$(CC) -c -o $@ $< -fPIC -I ../include/ $(CFLAGS) $(EXTRA)

bin/modules/%.so: obj/modules/%.o
	$(CC) -shared -o $@ $<

.PHONY: modules
modules: dirs $(MODULESOBJ)
