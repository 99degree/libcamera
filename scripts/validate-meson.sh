#!/bin/bash
# Validation script for meson.build changes

echo "=== Validating meson.build changes ==="

CHANGED_FILES=$(git diff --name-only 2>/dev/null | grep -E "meson\.build$" || true)

if [ -z "$CHANGED_FILES" ]; then
    echo "✅ No changes to meson.build files"
    exit 0
fi

echo "Changed files:"
echo "$CHANGED_FILES"
echo ""

ERRORS=0
for file in $CHANGED_FILES; do
    echo "Checking $file..."
    
    # Check for ONNX dependency if it's the softisp IPA module
    if [[ "$file" == *"softisp"*"meson.build"* ]]; then
        if ! grep -q "onnxruntime" "$file"; then
            echo "  ❌ ERROR: Missing ONNX Runtime dependency!"
            ERRORS=$((ERRORS + 1))
        else
            echo "  ✅ ONNX Runtime dependency present"
        fi
    fi
    
    # Check for balanced parentheses
    OPEN=$(grep -o "(" "$file" 2>/dev/null | wc -l)
    CLOSE=$(grep -o ")" "$file" 2>/dev/null | wc -l)
    if [ "$OPEN" -ne "$CLOSE" ]; then
        echo "  ❌ ERROR: Unbalanced parentheses ($OPEN open, $CLOSE close)"
        ERRORS=$((ERRORS + 1))
    else
        echo "  ✅ Parentheses balanced"
    fi
done

echo ""
if [ $ERRORS -gt 0 ]; then
    echo "❌ Validation failed with $ERRORS error(s)"
    exit 1
else
    echo "✅ All validations passed!"
    exit 0
fi
