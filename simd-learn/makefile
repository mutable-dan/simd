CXX=g++

TARGET=simd
ASMFILE=main.s
SRCFILE=main.cpp

CXXFLAGSALL     = -mavx2 -masm=att -Wall -Wextra -Wshadow -std=c++20 
CXXFLAGSRELEASE = $(CXXFLAGSALL) -O2 -DNDEBUG 
CXXFLAGSDEBUG   = $(CXXFLAGSALL) -g -O0 -DDEBUG

.PHONY: release
release: CFLAGS = $(CXXFLAGSRELEASE)
release: all

.PHONY: debug
debug: CFLAGS = $(CXXFLAGSDEBUG)
debug: all

all: $(TARGET)

$(TARGET): $(SRCFILE)
	$(CXX) $(CFLAGS) $(SRCFILE) -o $(TARGET)

asm: $(SRCFILE)
	$(CXX) $(CXXFLAGSALL) -O0 -S $(SRCFILE) -o $(ASMFILE)	

clean:
	rm -f $(TARGET) $(ASMFILE)
