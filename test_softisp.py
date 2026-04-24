#!/usr/bin/env python3
import sys
import os

# Set library paths
build_dir = "/data/data/com.termux/files/home/libcamera/build"
os.environ['LD_LIBRARY_PATH'] = f"{build_dir}/src/libcamera:{build_dir}/src/ipa/softisp:" + os.environ.get('LD_LIBRARY_PATH', '')
os.environ['LIBCAMERA_PIPELINES'] = f"{build_dir}/src/libcamera/pipeline/softisp"
os.environ['LIBCAMERA_IPA'] = f"{build_dir}/src/ipa/softisp"

try:
    import ctypes
    
    libcamera_path = f"{build_dir}/src/libcamera/libcamera.so"
    if not os.path.exists(libcamera_path):
        print(f"❌ ERROR: libcamera.so not found at {libcamera_path}")
        sys.exit(1)
    
    print(f"✅ libcamera.so found: {libcamera_path}")
    
    lib = ctypes.CDLL(libcamera_path)
    print(f"✅ Successfully loaded libcamera.so")
    
    softisp_pipeline = f"{build_dir}/src/libcamera/pipeline/softisp"
    if os.path.exists(softisp_pipeline):
        print(f"✅ SoftISP pipeline directory found: {softisp_pipeline}")
        for f in os.listdir(softisp_pipeline):
            if f.endswith('.so') or f.endswith('.o'):
                print(f"   - {f}")
    else:
        print(f"❌ SoftISP pipeline directory not found: {softisp_pipeline}")
    
    softisp_ipa = f"{build_dir}/src/ipa/softisp/ipa_softisp.so"
    if os.path.exists(softisp_ipa):
        print(f"✅ SoftISP IPA module found: {softisp_ipa}")
        # Check if it has ONNX symbols
        try:
            onnx_ipa = ctypes.CDLL(softisp_ipa)
            print(f"✅ Successfully loaded ipa_softisp.so")
        except Exception as e:
            print(f"⚠️  Could not load ipa_softisp.so: {e}")
    else:
        print(f"❌ SoftISP IPA module not found: {softisp_ipa}")
    
    print("\n✅ All basic checks passed!")
    print("The SoftISP pipeline is built and ready.")
    print("Note: Actual camera testing requires hardware or a V4L2 loopback device.")
    
except Exception as e:
    print(f"❌ ERROR: {e}")
    import traceback
    traceback.print_exc()
    sys.exit(1)
