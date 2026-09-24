.PHONY: all build run clean

APP_NAME = my_tui_app
BUILD_DIR = build

all: build

build:
	@mkdir -p $(BUILD_DIR)
	@go build -o $(BUILD_DIR)/$(APP_NAME) .

run: build
	@./$(BUILD_DIR)/$(APP_NAME)

clean:
	@rm -rf $(BUILD_DIR)
