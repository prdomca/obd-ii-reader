CXX := g++
CPPFLAGS := -Iinclude
DEFAULT_CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic
CXXFLAGS := $(DEFAULT_CXXFLAGS)
TARGET := build/obd_reader
LIBRARY_SOURCES := src/ObdConnection.cc src/ObdReader.cc src/ObdUtils.cc
SOURCES := src/main.cc $(LIBRARY_SOURCES)
OBJECTS := $(SOURCES:src/%.cc=build/%.o)
LIBRARY_OBJECTS := $(LIBRARY_SOURCES:src/%.cc=build/%.o)
TEST_SOURCES := tests/TestMain.cc tests/ObdUtilsTests.cc tests/ObdReaderTests.cc \
    tests/ObdConnectionTests.cc
TEST_OBJECTS := $(TEST_SOURCES:tests/%.cc=build/tests/%.o)
TEST_TARGET := build/obd_tests
DEPENDENCIES := $(OBJECTS:.o=.d)
TEST_DEPENDENCIES := $(TEST_OBJECTS:.o=.d)
COMPILE_DATABASE := build/compile_commands.json

.PHONY: all clean compile-database run-demo test test-unit test-integration test-sanitized

all: compile-database $(TARGET)

compile-database:
	@mkdir -p build
	@printf '[\n' > $(COMPILE_DATABASE)
	@first_entry=true; \
	for source in $(SOURCES) $(TEST_SOURCES); do \
		case "$$source" in \
			tests/*) object="build/tests/$$(basename "$$source" .cc).o" ;; \
			*) object="build/$$(basename "$$source" .cc).o" ;; \
		esac; \
		if [ "$$first_entry" = false ]; then printf ',\n' >> $(COMPILE_DATABASE); fi; \
		first_entry=false; \
		printf '  {\n    "directory": "%s",\n    "command": "%s",\n    "file": "%s",\n    "output": "%s"\n  }' \
			"$(CURDIR)" "$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $$source -o $$object" \
			"$$source" "$$object" >> $(COMPILE_DATABASE); \
	done
	@printf '\n]\n' >> $(COMPILE_DATABASE)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

$(TEST_TARGET): $(LIBRARY_OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(CXXFLAGS) $(LIBRARY_OBJECTS) $(TEST_OBJECTS) $(LDFLAGS) -o $@

build/%.o: src/%.cc
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

build/tests/%.o: tests/%.cc tests/TestFramework.h tests/TestSuites.h
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

run-demo: compile-database $(TARGET)
	printf 'data/demo.txt\n2\n1\n2\n3\n' | ./$(TARGET)

test: test-unit test-integration

test-unit: compile-database $(TEST_TARGET)
	./$(TEST_TARGET)

test-integration: compile-database $(TARGET)
	sh tests/demo_integration.sh ./$(TARGET)

test-sanitized:
	$(MAKE) clean
	$(MAKE) CXX=clang++ \
		CXXFLAGS='$(DEFAULT_CXXFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer' test
	$(MAKE) clean
	$(MAKE)

clean:
	rm -rf build

-include $(DEPENDENCIES) $(TEST_DEPENDENCIES)
