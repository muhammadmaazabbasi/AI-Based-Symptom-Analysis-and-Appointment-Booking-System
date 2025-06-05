# MediCare AI - Pure C++ Backend Makefile
# Compiles the complete OOP medical consultation system

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LIBS = -lcurl
TARGET = medicare_server
SOURCES = main.cpp MediCareServer.cpp

# Default target
all: $(TARGET)

# Build the medical server
$(TARGET): $(SOURCES) MediCareServer.h
	@echo "🏗️  Compiling MediCare AI C++ Backend..."
	@echo "✅ Using C++17 with full OOP features"
	@echo "✅ Linking with libcurl for Gemini AI integration"
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)
	@echo "✅ Build complete! Run with: ./$(TARGET)"

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build files..."
	rm -f $(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "📦 Installing required dependencies..."
	sudo apt-get update
	sudo apt-get install -y build-essential libcurl4-openssl-dev

# Run the server
run: $(TARGET)
	@echo "🚀 Starting MediCare AI Server..."
	./$(TARGET)

# Development target with debug symbols
debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

.PHONY: all clean install-deps run debug