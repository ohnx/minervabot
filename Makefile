CFLAGS+=-Wall -Werror -Iinclude/ -Ilib/mbedtls/include/
LDFLAGS=-ldl -Llib/ -lmbedtls -lmbedx509 -lmbedcrypto
OUTPUT=minervabot

# files
SRCS=$(wildcard src/*.c)
OBJS=$(patsubst src/%.c, objs/%.o, $(SRCS))
SRCS_MOD=$(wildcard src/modules/*.c)
OUTP_MOD=$(patsubst src/%.c, %.so, $(SRCS_MOD))

# primary rules
default: objs/modules/ modules/ $(OUTPUT) $(OUTP_MOD)
debug: CFLAGS+=-g -O0
debug: default

asan: CFLAGS+=-fsanitize=address
asan: LDFLAGS+=-fsanitize=address
asan: debug

# directories
objs/:
	@mkdir -p $@

objs/modules/: objs/
	@mkdir -p $@

modules/:
	@mkdir -p $@

# regular code
objs/%.o: src/%.c
	@echo "  CC\t$@"
	@$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT): $(OBJS)
	@echo "  CCLD\t$@"
	@gcc $^ $(LDFLAGS) -o $@

# modules
modules/%.so: src/modules/%.c
	@echo "  CCMOD\t$@"
	@$(CC) $< $(CFLAGS) -fPIC $(LDFLAGS) -shared -o $@

# misc
.PHONY: clean
clean:
	@echo "  RM\tobjs/ modules/"
	@rm -rf objs/ modules/ > /dev/null 2>&1 || true
	@echo "  RM\t$(OUTPUT)"
	@rm $(OUTPUT) > /dev/null 2>&1 || true

.PHONY: test
test: clean debug
	valgrind --leak-check=full --show-leak-kinds=all ./$(OUTPUT) "#="

.PHONY: testa
testa: clean asan
	./$(OUTPUT) "#="

# libraries
lib/libmbedtls.a:
	-@git submodule update --init --recursive
	$(MAKE) no_test -C lib/mbedtls
	cp lib/mbedtls/library/*.a lib/
