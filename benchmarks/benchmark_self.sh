#!/bin/bash
# Copyright 2025 Yi-Ping Pan (Cloudlet)
#
# Quick benchmark on Coogle's own codebase

set -e

BENCHMARK_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$BENCHMARK_DIR")"
COOGLE="${PROJECT_ROOT}/build/coogle"
RESULTS_FILE="${BENCHMARK_DIR}/results_self.txt"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Coogle Benchmark: Self (Coogle Codebase) ===${NC}\n"

# Check if coogle is built
if [ ! -f "$COOGLE" ]; then
    echo -e "${YELLOW}Building Coogle...${NC}"
    cd "$PROJECT_ROOT/build"
    cmake --build .
    cd "$BENCHMARK_DIR"
fi

# Count files (only in src/ and include/ to match what we search)
echo -e "${GREEN}Analyzing codebase...${NC}"
TOTAL_FILES=$(find "$PROJECT_ROOT/src" "$PROJECT_ROOT/include" -type f \( -name "*.cpp" -o -name "*.h" \) 2>/dev/null | wc -l | tr -d ' ')
echo "Total C++ files: $TOTAL_FILES"

# Test queries on Coogle's own code
declare -a QUERIES=(
    "void()"
    "std::string_view(std::string_view)"
    "bool(const coogle::Signature &, const coogle::Signature &)"
    "std::optional<*>(*)"
    "CXChildVisitResult(*)"
)

# Clear results file
echo "=== Coogle Benchmark on Self ===" > "$RESULTS_FILE"
echo "Date: $(date)" >> "$RESULTS_FILE"
echo "Files: $TOTAL_FILES" >> "$RESULTS_FILE"
echo "" >> "$RESULTS_FILE"

# Run benchmarks
for QUERY in "${QUERIES[@]}"; do
    echo -e "\n${GREEN}Testing query: ${YELLOW}$QUERY${NC}"
    echo "Query: $QUERY" >> "$RESULTS_FILE"

    # Measure time (search only src/ and include/ to avoid build artifacts)
    START=$(date +%s)
    OUTPUT=$("$COOGLE" "$PROJECT_ROOT/src" "$QUERY" 2>/dev/null)
    MATCH_COUNT=$(echo "$OUTPUT" | grep -c "^  " || true)
    END=$(date +%s)

    DURATION=$((END - START))

    echo "  Matches: $MATCH_COUNT"
    echo "  Time: ${DURATION}s"
    echo "  Matches: $MATCH_COUNT" >> "$RESULTS_FILE"
    echo "  Time: ${DURATION}s" >> "$RESULTS_FILE"
    echo "" >> "$RESULTS_FILE"

    # Show sample matches
    if [ $MATCH_COUNT -gt 0 ]; then
        echo "$OUTPUT" | head -10
    fi
done

# Memory usage test
echo -e "\n${GREEN}Measuring memory usage...${NC}"
echo "Memory Usage Test:" >> "$RESULTS_FILE"

if [ "$(uname)" = "Darwin" ]; then
    # macOS
    /usr/bin/time -l "$COOGLE" "$PROJECT_ROOT/src" "void()" > /dev/null 2> /tmp/coogle_time.txt
    MAX_RSS=$(grep "maximum resident set size" /tmp/coogle_time.txt | awk '{print $1}')
    MAX_RSS_MB=$((MAX_RSS / 1024 / 1024))
    echo "  Max RSS: ~${MAX_RSS_MB} MB"
    echo "  Max RSS: ~${MAX_RSS_MB} MB" >> "$RESULTS_FILE"
else
    # Linux
    /usr/bin/time -v "$COOGLE" "$PROJECT_ROOT/src" "void()" > /dev/null 2> /tmp/coogle_time.txt
    MAX_RSS=$(grep "Maximum resident set size" /tmp/coogle_time.txt | awk '{print $NF}')
    MAX_RSS_MB=$((MAX_RSS / 1024))
    echo "  Max RSS: ~${MAX_RSS_MB} MB"
    echo "  Max RSS: ~${MAX_RSS_MB} MB" >> "$RESULTS_FILE"
fi

echo -e "\n${BLUE}=== Benchmark Complete ===${NC}"
echo -e "Results saved to: ${YELLOW}$RESULTS_FILE${NC}\n"

cat "$RESULTS_FILE"
