OUT := bootimgtool

all: $(OUT)
LIBS_OPENSSL := $(shell pkg-config --libs openssl)

$(OUT): bootimgtool.o create_image.o
	gcc -O3 bootimgtool.o create_image.o  $(LIBS_OPENSSL) -o $(OUT)

bootimgtool.o: bootimgtool.c create_image.c
	gcc -c bootimgtool.c -o bootimgtool.o

create_image.o: create_image.c
	gcc -c create_image.c -o create_image.o
clean:
	@rm -rf *.o
	@rm -rf $(OUT)

install:
	install -m 755 $(OUT) /usr/bin
