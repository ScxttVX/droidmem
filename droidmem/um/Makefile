# droidmem - Usermode para leitura/escrita de memória
# Compilar: make (usa aarch64-linux-gnu- por padrão)

CROSS_COMPILE ?= aarch64-linux-gnu-
CC := $(CROSS_COMPILE)g++
STRIP := $(CROSS_COMPILE)strip

CFLAGS := -g -Wall -O2
TARGET := main
SRCS := $(wildcard *.cpp)
OBJS := $(patsubst %cpp,%o,$(SRCS))

all: $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -static -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -static -o $@ $^
	@echo "[+] Build OK: $(TARGET) — binário static aarch64"

clean:
	rm -rf $(TARGET) *.o

.PHONY: all clean
