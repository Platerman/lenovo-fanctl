CC      := gcc
CFLAGS  := -O2 -Wall -Wextra -DUNICODE -D_UNICODE -pthread
LDFLAGS := -mwindows -static -static-libgcc -s
LDLIBS  := -lshell32 -luser32 -lgdi32

TARGET  := lfc64.exe
SRCS    := src/lenovo_fan_control.c src/fanctrl.c
HDRS    := src/fanctrl.h

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRCS) $(LDLIBS)

clean:
	rm -f $(TARGET) *.o