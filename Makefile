CFLAGS+=-Wall -Werror -Iinclude/ -Ilib/mbedtls/include/
# not sure the best way to get usleep and signals and stuff other than D_BSD_SOURCE
CFLAGS+=-std=c99 -pedantic -D_BSD_SOURCE
MOD_CFLAGS+=-Ilib/jsmn/ -lcurl -Llib/ -ljsmn
LDFLAGS=-ldl -Llib/ -lmbedtls -lmbedx509 -lmbedcrypto -lpthread
OUTPUT=minervabot

# files
SRCS=$(wildcard src/*.c)
OBJS=$(patsubst src/%.c, objs/%.o, $(SRCS))
SRCS_MOD=$(wildcard src/modules/*.c)
OUTP_MOD=$(patsubst src/%.c, %.so, $(SRCS_MOD))
LIBS=lib/libmbedtls.a lib/libjsmn.a

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

$(OUTPUT): $(LIBS) $(OBJS)
	@echo "  CCLD\t$@"
	@gcc $^ $(LDFLAGS) -o $@

# modules
modules/%.so: src/modules/%.c
	@echo "  CCMOD\t$@"
	@$(CC) $< $(CFLAGS) $(MOD_CFLAGS) -fPIC $(LDFLAGS) -shared -o $@

modules/bot_math.so: src/modules/bot_math.c
	@echo "  CCMOD\t$@"
	@$(CC) $< $(CFLAGS) -fPIC $(LDFLAGS) -lleo -lm -shared -o $@ -Ilib/leo/include

LUA_V=5.3.5
lib/liblua.a:
	cd lib/; curl -R -O http://www.lua.org/ftp/lua-$(LUA_V).tar.gz; tar zxf lua-$(LUA_V).tar.gz
	cd lib/lua-$(LUA_V)/; make generic
	cp lib/lua-$(LUA_V)/src/liblua.a lib/

modules/bot_lua.so: src/modules/bot_lua.c lib/liblua.a
	@echo "  CCMOD\t$@"
	@$(CC) $< $(CFLAGS) -fPIC $(LDFLAGS) -llua -shared -o $@ -Ilib/lua-$(LUA_V)/src/

# misc
.PHONY: clean
clean:
	@echo "  RM\tobjs/ modules/"
	@rm -rf objs/ modules/ > /dev/null 2>&1 || true
	@echo "  RM\t$(OUTPUT)"
	@rm $(OUTPUT) > /dev/null 2>&1 || true

.PHONY: test
test: clean debug
	valgrind --tool=massif ./$(OUTPUT) "#/"
	#--leak-check=full --show-leak-kinds=all 
	#--tool=massif

.PHONY: testa
testa: clean asan
	./$(OUTPUT) "#/"

# libraries
lib/libmbedtls.a:
	-@git submodule update --init --recursive
	$(MAKE) no_test -C lib/mbedtls
	cp lib/mbedtls/library/*.a lib/

lib/libjsmn.a:
	-@git submodule update --init --recursive
	CFLAGS=-fPIC $(MAKE) all -C lib/jsmn
	cp lib/jsmn/*.a lib/
