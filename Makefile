CC=g++
CCFLAGS=-c
LDFLAGS=
SOURCES=main.cpp magnet.cpp chararray.cpp

ifeq ($(OS),Windows_NT)
    CCFLAGS += -D WIN32
	SOURCES += sha1.o
else
	LDFLAGS += -lgcrypt
endif

OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=magnet

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CCFLAGS) $< -o $@

clean:
	rm -rf *.o magnet
