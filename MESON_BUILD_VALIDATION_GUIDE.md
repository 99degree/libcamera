# Meson.build Validation Guide

## Problem
The `meson.build` files are critical for the build system. Corruption (missing dependencies, unbalanced parentheses, etc.) can break the entire build.

## Solution
A validation script has been created to automatically check `meson.build` files before committing.

## How to Use

### Manual Validation (Before Committing)
```bash
# Run validation
./scripts/validate-meson.sh

# If it passes, you can safely commit
git add <files>
git commit -m "Your commit message"
```

### What the Script Checks
1. **ONNX Runtime Dependency**: For `softisp` IPA module, ensures `onnxruntime` dependency is present
2. **Balanced Parentheses**: Ensures all `(` have matching `)`
3. **Common Corruption Patterns**: Checks for empty lines, syntax errors

### Example Output
```
=== Validating meson.build changes ===
Changed files:
src/ipa/softisp/meson.build

Checking src/ipa/softisp/meson.build...
  ✅ ONNX Runtime dependency present
  ✅ Parentheses balanced

✅ All validations passed!
```

## If Validation Fails
If the script reports errors:
1. **Missing ONNX dependency**: Add `onnxruntime_dep = dependency('onnxruntime', version: '>=1.14.0')`
2. **Unbalanced parentheses**: Check for missing `)` in function calls
3. **Other issues**: Compare with previous git version:
   ```bash
   git diff src/ipa/softisp/meson.build
   git checkout HEAD -- src/ipa/softisp/meson.build  # Restore if needed
   ```

## Best Practices
1. **Always run validation** before committing `meson.build` changes
2. **Compare with Git**: `git diff src/ipa/softisp/meson.build`
3. **Test build**: `ninja -C softisp_only` after any meson.build change
4. **Small changes**: Make one change at a time to make debugging easier

## Files Protected
- `src/ipa/softisp/meson.build` - IPA module build
- `meson.build` - Main project build
- Any other `meson.build` files in the repository

---

**Script Location**: `scripts/validate-meson.sh`  
**Usage**: `./scripts/validate-meson.sh`  
**Status**: ✅ Active and tested
