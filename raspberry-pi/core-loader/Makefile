CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = core3_loader_v2

all: $(TARGET)

$(TARGET): core3_loader_v2.c
	$(CC) $(CFLAGS) -o $(TARGET) core3_loader_v2.c

clean:
	rm -f $(TARGET)

install: $(TARGET)
	@echo "Run with: sudo ./$(TARGET) <kernel_core3.img>"

.PHONY: all clean install
