#!/bin/bash
# Copyright 2025 Yi-Ping Pan (Cloudlet)
#
# Benchmark Coogle on LLVM codebase

set -e

BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$BENCHMARK_DIR")"
COOGLE="${PROJECT_ROOT}/build/coogle"
LLVM_DIR="${BENCHMARK_DIR}/llvm-project"
RESULTS_FILE="${BENCHMARK_DIR}/results_llvm.txt"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Coogle Benchmark: LLVM Project ===${NC}\n"

# Check if coogle is built
if [ ! -f "$COOGLE" ]; then
    echo -e "${YELLOW}Building Coogle...${NC}"
    cd "$PROJECT_ROOT/build"
    cmake --build .
    cd "$BENCHMARK_DIR"
fi

# Clone LLVM if not present
if [ ! -d "$LLVM_DIR" ]; then
    echo -e "${YELLOW}Cloning LLVM project (this may take a while)...${NC}"
    git clone --depth 1 --branch release/18.x https://github.com/llvm/llvm-project.git
fi

# Count files and estimate functions
echo -e "${GREEN}Analyzing codebase...${NC}"
TOTAL_FILES=$(find "$LLVM_DIR/llvm" -type f \( -name "*.cpp" -o -name "*.h" \) | wc -l | tr -d ' ')
echo "Total C++ files: $TOTAL_FILES"

# Test queries
declare -a QUERIES=(
    "bool(Function &)"
    "void(llvm::raw_ostream &)"
    "bool(llvm::StringRef)"
    "llvm::Value *(llvm::Type *)"
    "void()"
    "int(int, int)"
)

# Clear results file
echo "=== Coogle Benchmark on LLVM ===" > "$RESULTS_FILE"
echo "Date: $(date)" >> "$RESULTS_FILE"
echo "Files: $TOTAL_FILES" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Run benchmarks
for QUERY in "${QUERIES[@]}"; do
    echo -e "\n${GREEN}Testing query: ${YELLOW}$QUERY${NC}"
    echo "Query: $QUERY" >> "$RESULTS_FILE"

    # Measure time using time command
    START=$(date +%s.%N)
    MATCH_COUNT=$("$COOGLE" "$LLVM_DIR/llvm" "$QUERY" 2>/dev/null | grep -c "^  " || true)
    END=$(date +%s.%N)

    DURATION=$(echo "$END - $START" | bc)

    echo "  Matches: $MATCH_COUNT"
    echo "  Time: ${DURATION}s"
    echo "  Matches: $MATCH_COUNT" >> "$RESULTS_FILE"
    echo "  Time: ${DURATION}s" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"
done

# Memory usage test
echo -e "\n${GREEN}Measuring memory usage...${NC}"
echo "Memory Usage Test:" >> "$RESULTS_FILE"

# Use /usr/bin/time on macOS or GNU time on Linux
if [ "$(uname)" = "Darwin" ]; then
    # macOS
    /usr/bin/time -l "$COOGLE" "$LLVM_DIR/llvm" "void()" > /dev/null 2> /tmp/coogle_time.txt
    MAX_RSS=$(grep "maximum resident set size" /tmp/coogle_time.txt | awk '{print $1}')
    MAX_RSS_MB=$(echo "scale=2; $MAX_RSS / 1024 / 1024" | bc)
    echo "  Max RSS: ${MAX_RSS_MB} MB"
    echo "  Max RSS: ${MAX_RSS_MB} MB" >> "$RESULTS_FILE"
else
    # Linux
    /usr/bin/time -v "$COOGLE" "$LLVM_DIR/llvm" "void()" > /dev/null 2> /tmp/coogle_time.txt
    MAX_RSS=$(grep "Maximum resident set size" /tmp/coogle_time.txt | awk '{print $NF}')
    MAX_RSS_MB=$(echo "scale=2; $MAX_RSS / 1024" | bc)
    echo "  Max RSS: ${MAX_RSS_MB} MB"
    echo "  Max RSS: ${MAX_RSS_MB} MB" >> "$RESULTS_FILE"
fi

echo -e "\n${BLUE}=== Benchmark Complete ===${NC}"
echo -e "Results saved to: ${YELLOW}$RESULTS_FILE${NC}\n"

cat "$RESULTS_FILE"
