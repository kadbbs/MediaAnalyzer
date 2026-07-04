GO ?= /usr/local/go/bin/go

.PHONY: build-cpp test test-go build-go run-core run-go clean

build-cpp:
	cmake -S . -B build
	cmake --build build

test: build-cpp
	python3 tools/golden_test.py

test-go:
	cd server && $(GO) test ./...

build-go:
	cd server && $(GO) build ./cmd/media-analyzer-server

run-core: build-cpp
	./build/media-analyzer-core $(FILE)

run-go:
	cd server && $(GO) run ./cmd/media-analyzer-server

clean:
	rm -rf build
