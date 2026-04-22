#!/usr/bin/env python3
"""
Simple SoftISP frame capture script using libcamera Python bindings.
Saves processed frames to disk.
"""

import os
import sys
import time
import argparse
from libcamera import CameraManager, controls

def capture_frames(camera_id, count, output_dir, timeout=10):
    """Capture frames from SoftISP camera and save to disk."""
    
    # Create output directory
    os.makedirs(output_dir, exist_ok=True)
    
    # Initialize camera manager
    cm = CameraManager()
    cm.start()
    
    # Find the SoftISP camera
    cameras = cm.cameras
    target_camera = None
    for cam in cameras:
        if camera_id in cam.id:
            target_camera = cam
            break
    
    if not target_camera:
        print(f"Camera not found: {camera_id}")
        cm.stop()
        return False
    
    print(f"Using camera: {target_camera.id}")
    
    # Configure camera (FHD default)
    config = target_camera.generate_configuration(["raw"])
    if config.validate() != 1:  # Valid
        print("Invalid configuration")
        return False
    
    target_camera.configure(config)
    
    stream = config.at(0).stream
    width = stream.configuration.size.width
    height = stream.configuration.size.height
    
    print(f"Stream: {width}x{height}")
    
    # Allocate buffers
    allocator = target_camera.frame_buffer_allocator(stream)
    buffers = []
    for _ in range(config.at(0).buffer_count):
        buffer = allocator.allocate(stream)
        if buffer:
            buffers.append(buffer)
    
    if len(buffers) == 0:
        print("Failed to allocate buffers")
        return False
    
    # Create requests
    requests = []
    for buffer in buffers:
        request = target_camera.create_request()
        if request.add_buffer(stream, buffer) < 0:
            print("Failed to add buffer to request")
            return False
        requests.append(request)
    
    # Start camera
    if target_camera.start() < 0:
        print("Failed to start camera")
        return False
    
    print(f"Capturing {count} frames...")
    
    # Capture frames
    completed = 0
    for i in range(count):
        request = requests[i % len(requests)]
        
        if target_camera.queue_request(request) < 0:
            print(f"Failed to queue request {i}")
            break
        
        # Wait for completion (simple polling)
        start_time = time.time()
        while request.status() != 2:  # RequestComplete
            time.sleep(0.01)
            if time.time() - start_time > timeout:
                print(f"Timeout waiting for frame {i}")
                break
        
        if request.status() == 2:  # Complete
            # Save frame
            buffer = request.buffers()[stream]
            plane = buffer.planes[0]
            
            # Read buffer data
            fd = plane.fd
            length = plane.length
            
            # Map memory
            import mmap
            with open(f'/dev/fd/{fd}', 'rb') as f:
                data = f.read(length)
            
            # Save to file
            filename = os.path.join(output_dir, f"frame_{i:03d}.bin")
            with open(filename, 'wb') as f:
                f.write(data)
            
            print(f"Saved frame {i} to {filename} ({len(data)} bytes)")
            completed += 1
            
            # Reuse request
            request.reuse()
    
    # Cleanup
    target_camera.stop()
    cm.stop()
    
    print(f"Captured {completed}/{count} frames successfully")
    return completed > 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Capture frames from SoftISP camera")
    parser.add_argument("-c", "--camera", default="SoftISP", help="Camera ID substring")
    parser.add_argument("-n", "--count", type=int, default=3, help="Number of frames")
    parser.add_argument("-o", "--output", default="./softisp_frames", help="Output directory")
    
    args = parser.parse_args()
    
    success = capture_frames(args.camera, args.count, args.output)
    sys.exit(0 if success else 1)
