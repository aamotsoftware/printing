CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0`
LIBS = `pkg-config --libs gtk+-3.0`
TARGET = print_app

all: $(TARGET)

$(TARGET): print_app.c
	$(CC) $(CFLAGS) -o $(TARGET) print_app.c $(LIBS)

clean:
	rm -f $(TARGET)

