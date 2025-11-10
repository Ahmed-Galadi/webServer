#!/bin/bash

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "=========================================="
echo "  Nginx Behavior Matrix Test Suite"
echo "=========================================="
echo ""

# Kill any existing webserv instances
pkill -9 webserv 2>/dev/null
sleep 1

# Array of test cases: "num:description:expected_code:expected_content:url_path"
declare -a tests=(
    "1:Index exists + autoindex on:200:TBI Index File:/cgi-bin/"
    "2:Index exists + autoindex off:200:TBI Index File:/cgi-bin/"
    "3:Index not found + autoindex on:200:Index of:/cgi-bin/"
    "4:Index not found + autoindex off:403:403 Forbidden:/cgi-bin/"
    "5:Directory doesn't exist + autoindex on:404:404 Not Found:/nonexistent-dir/"
    "6:Directory doesn't exist + autoindex off:404:404 Not Found:/nonexistent-dir/"
    "7:Path is file not directory:200:This is a file, not a directory:/fileasdir"
    "8:Index file no read permission:403:403 Forbidden:/noperm/"
    "9:Index file not found + autoindex on:200:Index of:/multiindex/"
    "10:Index file not found + autoindex off:403:403 Forbidden:/multiindex/"
    "11:No index directive + autoindex on:200:Index of:/cgi-bin/"
    "12:No index directive + autoindex off:403:403 Forbidden:/cgi-bin/"
)

# Function to run a test
run_test() {
    local test_num=$1
    local description=$2
    local expected_code=$3
    local expected_content=$4
    local url_path=$5
    
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    echo -e "${YELLOW}Test Case $test_num: $description${NC}"
    echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
    
    # Start webserv in background
    ./webserv config/test${test_num}.conf > /dev/null 2>&1 &
    local pid=$!
    
    # Wait for server to start
    sleep 2
    
    # Make request and capture response
    local response=$(curl -s -w "\nHTTP_CODE:%{http_code}" http://localhost:8080${url_path})
    local http_code=$(echo "$response" | grep "HTTP_CODE:" | cut -d: -f2)
    local body=$(echo "$response" | sed '/HTTP_CODE:/d')
    
    # Kill the server
    kill -9 $pid 2>/dev/null
    wait $pid 2>/dev/null
    sleep 1
    
    # Check result
    echo -e "Expected: ${YELLOW}HTTP $expected_code${NC}"
    echo -e "Received: ${YELLOW}HTTP $http_code${NC}"
    
    if [ "$http_code" == "$expected_code" ]; then
        # Check content
        if echo "$body" | grep -q "$expected_content"; then
            echo -e "Content:  ${GREEN}✓ Contains \"$expected_content\"${NC}"
            echo -e "Result:   ${GREEN}✓ PASS${NC}"
        else
            echo -e "Content:  ${RED}✗ Missing \"$expected_content\"${NC}"
            echo -e "Result:   ${RED}✗ FAIL${NC}"
            echo -e "${YELLOW}Response body:${NC}"
            echo "$body" | head -n 5
        fi
    else
        echo -e "Result:   ${RED}✗ FAIL (Wrong HTTP code)${NC}"
        echo -e "${YELLOW}Response body (first 5 lines):${NC}"
        echo "$body" | head -n 5
    fi
    
    echo ""
}

# Run all tests
for test in "${tests[@]}"; do
    IFS=':' read -r num desc code content url <<< "$test"
    run_test "$num" "$desc" "$code" "$content" "$url"
done

# Final cleanup
pkill -9 webserv 2>/dev/null

echo "=========================================="
echo "  Test Suite Complete"
echo "=========================================="
