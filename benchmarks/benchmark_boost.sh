#!/bin/bash
# Copyright 2025 Yi-Ping Pan (Cloudlet)
#
# Benchmark Coogle on Boost codebase

set -e

BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$BENCHMARK_DIR")"
COOGLE="${PROJECT_ROOT}/build/coogle"
BOOST_DIR="${BENCHMARK_DIR}/boost-asio"
RESULTS_FILE="${BENCHMARK_DIR}/results_boost.txt"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Coogle Benchmark: Boost.Asio Library ===${NC}\n"

# Check if coogle is built
if [ ! -f "$COOGLE" ]; then
    echo -e "${YELLOW}Building Coogle...${NC}"
    cd "$PROJECT_ROOT/build"
    cmake --build .
    cd "$BENCHMARK_DIR"
fi

# Clone Boost.Asio if not present (smaller, single library)
if [ ! -d "$BOOST_DIR" ]; then
    echo -e "${YELLOW}Cloning Boost.Asio library...${NC}"
    git clone --depth 1 https://github.com/boostorg/asio.git boost-asio
fi

# Count files
echo -e "${GREEN}Analyzing codebase...${NC}"
TOTAL_FILES=$(find "$BOOST_DIR" -type f \( -name "*.cpp" -o -name "*.hpp" \) | wc -l | tr -d ' ')
echo "Total C++ files: $TOTAL_FILES"

# Test queries (Boost-specific patterns)
declare -a QUERIES=(
    "void(const boost::system::error_code &)"
    "bool(const std::string &)"
    "std::shared_ptr<*>()"
    "void(boost::asio::io_context &)"
    "int()"
)

# Clear results file
echo "=== Coogle Benchmark on Boost ===" > "$RESULTS_FILE"
echo "Date: $(date)" >> "$RESULTS_FILE"
echo "Files: $TOTAL_FILES" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Run benchmarks
for QUERY in "${QUERIES[@]}"; do
    echo -e "\n${GREEN}Testing query: ${YELLOW}$QUERY${NC}"
    echo "Query: $QUERY" >> "$RESULTS_FILE"

    # Measure time
    START=$(date +%s.%N)
    MATCH_COUNT=$("$COOGLE" "$BOOST_DIR" "$QUERY" 2>/dev/null | grep -c "^  " || true)
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

if [ "$(uname)" = "Darwin" ]; then
    # macOS
    /usr/bin/time -l "$COOGLE" "$BOOST_DIR" "void()" > /dev/null 2> /tmp/coogle_time.txt
    MAX_RSS=$(grep "maximum resident set size" /tmp/coogle_time.txt | awk '{print $1}')
    MAX_RSS_MB=$(echo "scale=2; $MAX_RSS / 1024 / 1024" | bc)
    echo "  Max RSS: ${MAX_RSS_MB} MB"
    echo "  Max RSS: ${MAX_RSS_MB} MB" >> "$RESULTS_FILE"
else
    # Linux
    /usr/bin/time -v "$COOGLE" "$BOOST_DIR" "void()" > /dev/null 2> /tmp/coogle_time.txt
    MAX_RSS=$(grep "Maximum resident set size" /tmp/coogle_time.txt | awk '{print $NF}')
    MAX_RSS_MB=$(echo "scale=2; $MAX_RSS / 1024" | bc)
    echo "  Max RSS: ${MAX_RSS_MB} MB"
    echo "  Max RSS: ${MAX_RSS_MB} MB" >> "$RESULTS_FILE"
fi

echo -e "\n${BLUE}=== Benchmark Complete ===${NC}"
echo -e "Results saved to: ${YELLOW}$RESULTS_FILE${NC}\n"

cat "$RESULTS_FILE"
