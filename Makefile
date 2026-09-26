CXX := g++
CPPFLAGS := -Iinclude
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic
TARGET := build/obd_reader
SOURCES := src/main.cc src/ObdConnection.cc src/ObdReader.cc src/ObdUtils.cc
OBJECTS := $(SOURCES:src/%.cc=build/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)
COMPILE_DATABASE := build/compile_commands.json

.PHONY: all clean compile-database run-demo

all: compile-database $(TARGET)

compile-database:
	@mkdir -p build
	@printf '[\n' > $(COMPILE_DATABASE)
	@first_entry=true; \
	for source in $(SOURCES); do \
		object="build/$$(basename "$$source" .cc).o"; \
		if [ "$$first_entry" = false ]; then printf ',\n' >> $(COMPILE_DATABASE); fi; \
		first_entry=false; \
		printf '  {\n    "directory": "%s",\n    "command": "%s",\n    "file": "%s",\n    "output": "%s"\n  }' \
			"$(CURDIR)" "$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $$source -o $$object" \
			"$$source" "$$object" >> $(COMPILE_DATABASE); \
	done
	@printf '\n]\n' >> $(COMPILE_DATABASE)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

run-demo: compile-database $(TARGET)
	printf 'data/demo.txt\n2\n1\n2\n3\n' | ./$(TARGET)

clean:
	rm -rf build

-include $(DEPENDENCIES)
