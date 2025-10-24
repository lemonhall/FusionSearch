#!/bin/bash
# Test Coverage Report Generator

echo "Generating test coverage report..."

# Modules to check
MODULES="trie tokenizer index vector_index search bm25 utils snippet file_loader"

echo "Module Coverage Summary:"
echo "========================"

for module in $MODULES; do
    if [ -f "../src/${module}.gcda" ]; then
        gcov -o ../src ../src/${module}.c > /dev/null 2>&1
        
        if [ -f "${module}.c.gcov" ]; then
            lines=$(grep -c "^[^-]" ${module}.c.gcov)
            executed=$(grep -c "^[ ]*[1-9]" ${module}.c.gcov)
            coverage=$((executed * 100 / lines))
            echo "${module}: ${coverage}% (${executed}/${lines})"
        fi
    fi
done

echo ""
echo "Detailed report saved to *.gcov files"
