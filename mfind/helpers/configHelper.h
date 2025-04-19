#pragma once

#include <string>
#include <vector>

// Chunk size for reading files
constexpr size_t DEFAULT_CHUNK_SIZE = 8192;

// Overlap size between chunks (should be >= max keyword length)
constexpr size_t DEFAULT_OVERLAP_SIZE = 100;

// Context characters to print before/after match
constexpr size_t CONTEXT_CHARS = 30;

inline const std::vector<std::string> DEFAULT_COLORS = {
    "\033[31m", // Red
    "\033[32m", // Green
    "\033[33m", // Yellow
    "\033[34m", // Blue
    "\033[35m", // Magenta
    "\033[36m", // Cyan
};

inline const std::string RESET_COLOR = "\033[0m";
