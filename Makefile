CFLAGS := -O3
CC := gcc
LDFLAGS := $(shell pkg-config --libs openssl)
OBJS := create_image.o bootimgtool.o
OUT := bootimgtool

ifeq ($(OS),Windows_NT)
	OBJS += win32.o
	CFLAGS += -static
endif

all: $(OUT)

$(OUT): $(OBJS)
	gcc $(OBJS) $(LDFLAGS) -o $(OUT)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	@rm -rf *.o
	@rm -rf $(OUT)

install: $(OUT)
	@install -m 755 $(OUT) /usr/bin
