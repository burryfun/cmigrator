CC=gcc
SRC=main.c
OUT=main.exe

CFLAGS = -Wall -g
INCLUDEDIR := $(shell pg_config --includedir)
LIBDIR := $(shell pg_config --libdir)

$(OUT): $(SRC)
	$(CC) $(SRC) -I"$(INCLUDEDIR)" $(CFLAGS) -L"$(LIBDIR)" -lpq -o $(OUT) 

clean:
	rm -rf $(OUT)
