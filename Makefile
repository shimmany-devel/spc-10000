BUILD_DIR ?= build

.PHONY: all build clean run test-headless

all: build

$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build: $(BUILD_DIR)/CMakeCache.txt
	cmake --build $(BUILD_DIR) -j$$(nproc 2>/dev/null || echo 4)

run: build
	./$(BUILD_DIR)/spc_launcher

test-headless: build
	./$(BUILD_DIR)/spc_launcher --headless 120

clean:
	rm -rf $(BUILD_DIR)
