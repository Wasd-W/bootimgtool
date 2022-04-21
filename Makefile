OUT := bootimgtool
LIBS_OPENSSL := $(shell pkg-config --libs openssl)
OBJS := create_image.o bootimgtool.o

ifeq ($(OS),Windows_NT)
	OBJS += win32.o
endif

all: $(OUT)

$(OUT): $(OBJS)
	gcc -O3 $(OBJS)  $(LIBS_OPENSSL) -o $(OUT)

bootimgtool.o: bootimgtool.c create_image.c
	gcc -c bootimgtool.c -o bootimgtool.o

create_image.o: create_image.c
	gcc -c create_image.c -o create_image.o

win32.o: win32.c
	gcc -c win32.c -o win32.o

clean:
	@rm -rf *.o
	@rm -rf $(OUT)

install: $(OUT)
	@install -m 755 $(OUT) /usr/bin
