BUILD_DIR ?= build

.PHONY: all build clean run

all: build

$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR) -j$$(nproc 2>/dev/null || echo 4)

run: build
	./$(BUILD_DIR)/bin/spc_launcher

clean:
	rm -rf $(BUILD_DIR)
