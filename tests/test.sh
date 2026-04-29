#!/bin/bash

BINARY="./bin/memsim"
INPUT="tests/input.txt"
EXPECTED="tests/expected.txt"

if [ ! -f "$BINARY" ]; then
    echo "Error: $BINARY not found. Run 'make' first."
    exit 1
fi

# Run program, strip '> ' prompts and welcome message, filter blank lines
actual=$($BINARY 8192 < $INPUT 2>/dev/null \
    | sed 's/^\(> \)*//' \
    | grep -v "^Welcome" \
    | grep -v "^Commands" \
    | grep -v "^  \*" \
    | grep -v "^    \*" \
    | grep -v "^$")

echo "$actual" > tests/actual.txt

diff <(cat $EXPECTED) <(echo "$actual")

if [ $? -eq 0 ]; then
    echo "All tests passed!"
else
    echo "Differences found (- expected, + actual)"
    echo "Full actual output saved to tests/actual.txt"
    exit 1
fi
