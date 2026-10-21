BUILD_DIR ?= build

.PHONY: all build clean run test-headless

all: build

$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR) -j$$(nproc 2>/dev/null || echo 4)

run: run-spc1000

run-spc1000: build
	./$(BUILD_DIR)/spc_launcher

run-spc10000: build
	python3 firmware-spc10000/build_rom.py
	./$(BUILD_DIR)/spc_launcher --spc10000

test-headless: build
	./$(BUILD_DIR)/spc_launcher --headless 120

test-z8000: build
	./$(BUILD_DIR)/spc_launcher --test-z8000

test-spc10000: build
	python3 firmware-spc10000/build_rom.py
	./$(BUILD_DIR)/spc_launcher --spc10000 --headless 60

fpga-project:
	make -C fpga-zcu102 project

clean:
	rm -rf $(BUILD_DIR)
