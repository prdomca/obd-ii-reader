CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic
TARGET := obd_reader
SOURCES := main.cc ObdConnection.cc ObdReader.cc ObdUtils.cc

.PHONY: all clean run-demo

all: $(TARGET)

$(TARGET): $(SOURCES) ObdConnection.h ObdException.h ObdReader.h ObdUtils.h
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET)

run-demo: $(TARGET)
	printf 'demo.txt\n2\n1\n2\n3\n' | ./$(TARGET)

clean:
	rm -f $(TARGET) *.o
