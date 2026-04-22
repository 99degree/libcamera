# SoftISP Wrapper Generator - Skills Guide

## Problem
When using Mojo IPC in libcamera, the generated serialization code uses generic parameter names like `t1`, `t2`, `t3` instead of the meaningful names defined in the `.mojom` source file. This makes debugging and code maintenance difficult.

## Solution
A **custom wrapper generator** that:
1. Reads the `.mojom` file using regex parsing (no complex AST needed)
2. Generates a C++ wrapper class with clean, named parameters
3. Automatically maps named parameters to the internal `t1/t2` serialization
4. Regenerates whenever the `.mojom` file changes (via Meson)

## Architecture

### Files Created
- `utils/codegen/ipc/generators/softisp_wrapper_generator.py` - The generator script
- `src/ipa/softisp/meson.build` - Updated to run the generator
- `build/src/ipa/softisp/softisp_wrapper.h` - Generated wrapper (auto-created)

### How It Works

1. **Meson Build Integration**:
   ```meson
   wrapper_gen = custom_target('softisp_wrapper',
     input : mojom_file,
     output : 'softisp_wrapper.h',
     command : [python3, generator_script, '--mojom-file', '@INPUT@', '--output-dir', '@OUTDIR@'],
   )
   ```

2. **Generator Logic**:
   - Uses regex to parse `.mojom` interface definitions
   - Extracts method names, parameter types, and parameter names
   - Generates C++ wrapper with clean API
   - Adds comments showing `param (t1)` mapping

3. **Usage in Code**:
   ```cpp
   #include "softisp_wrapper.h"
   
   // Create wrapper once (e.g., in init())
   auto wrapper = std::make_unique<SoftIspInterfaceWrapper>(std::move(interface));
   
   // Use clean names - no t1/t2!
   wrapper->configure(1920, 1080, 0);  // Clear and readable!
   wrapper->processFrame(frameId, inputBuf, outputBuf);
   ```

## Key Features

### 1. Robust Path Handling
Uses `files()` in Meson to resolve paths relative to project root, avoiding fragile `../../../` patterns.

### 2. Automatic Regeneration
When `.mojom` changes, Meson detects the timestamp change and re-runs the generator before compiling.

### 3. Simple Parsing
Uses regex instead of full mojom AST parser, making it:
- Easier to maintain
- Faster to execute
- Less dependent on internal mojom APIs

### 4. Clear Documentation
Generated wrapper includes comments showing the mapping:
```cpp
/**
 * configure
 * Mapping: width (t1), height (t2), format (t3)
 */
int32 configure(uint32 width, uint32 height, uint32 format) {
    return interface_->configure(width, height, format);
}
```

## Benefits

1. **No More t1/t2 Confusion**: Your code uses meaningful names
2. **Type Safety**: Compiler ensures correct parameter types
3. **Auto-Updated**: Wrapper regenerates when `.mojom` changes
4. **Zero Runtime Overhead**: Wrapper is just a thin C++ class (pointer pass-through)
5. **Initialize Once**: Create wrapper during camera init, not per-frame

## Maintenance

### Adding New Methods
1. Add method to `.mojom` file
2. Run `meson compile` - wrapper regenerates automatically
3. Use new method in code with clean names

### Modifying Parameters
1. Change parameter names in `.mojom`
2. Rebuild - wrapper updates with new names
3. No manual sync needed!

## Testing
```bash
# Build with wrapper generation
meson setup build -Dsoftisp=enabled -Dpipelines='softisp,dummysoftisp' -Dtest=true
ninja -C build src/ipa/softisp/ipa_softisp.so

# Verify wrapper was generated
cat build/src/ipa/softisp/softisp_wrapper.h

# Run test app
export LD_LIBRARY_PATH=build/src/libcamera:build/src/ipa/dummysoftisp:$LD_LIBRARY_PATH
export SOFTISP_MODEL_DIR=/path/to/models
./build/tools/softisp-test-app --pipeline dummysoftisp --frames 1
```

## Future Enhancements
- Extend generator to support multiple interfaces in one `.mojom` file
- Add optional argument validation in wrapper
- Generate both C++ and Python wrappers from same `.mojom`
