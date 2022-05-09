CFLAGS := -O3
LDFLAGS := $(shell pkg-config --libs openssl)
OBJS := create_image.o bootimgtool.o
OUT := bootimgtool

ifeq ($(OS),Windows_NT)
	OBJS += win32.o
	CFLAGS += -static
endif

all: $(OUT)

$(OUT): $(OBJS)
	gcc $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(OUT)

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
