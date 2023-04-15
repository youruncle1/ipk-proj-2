CXX=g++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++20
LDFLAGS=-pthread
TARGET=ipkcpd

.PHONY: all clean

all: $(TARGET)

$(TARGET): ipkcpd.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) ipkcpd.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)