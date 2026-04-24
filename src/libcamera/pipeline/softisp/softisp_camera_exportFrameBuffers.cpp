#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

int SoftISPCameraData::exportFrameBuffers([[maybe_unused]] Stream *stream,
                                          std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    LOG(SoftISPPipeline, Info) << "Exporting frame buffers (fallback mode)";
    
    if (!buffers) {
        LOG(SoftISPPipeline, Error) << "Null buffers vector";
        return -EINVAL;
    }
    
    if (!virtualCamera("default")) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }
    
    // Get buffer count from VirtualCamera
    unsigned int count = virtualCamera("default")->bufferCount();
    unsigned int width = virtualCamera("default")->width();
    unsigned int height = virtualCamera("default")->height();
    
    // Calculate buffer size for Bayer10 (10 bits per pixel, packed)
    size_t bufferSize = (width * height * 5) / 4;
    
    LOG(SoftISPPipeline, Info) << "Creating " << count << " buffers of size " << bufferSize
                               << " for " << width << "x" << height;
    
    buffers->clear();
    
    for (unsigned int i = 0; i < count; ++i) {
        // Create a temporary FD using memfd_create or fallback to /dev/zero
        int fd = -1;
        
        // Try memfd_create first (Linux 3.17+)
        #ifdef __linux__
        fd = memfd_create("softisp_buffer", MFD_CLOEXEC);
        #endif
        
        if (fd < 0) {
            // Fallback to /dev/zero
            LOG(SoftISPPipeline, Info) << "memfd_create failed, using /dev/zero for buffer " << i;
            fd = open("/dev/zero", O_RDWR | O_CLOEXEC);
        }
        
        if (fd < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to create buffer " << i;
            return -errno;
        }
        
        // Set the size
        if (ftruncate(fd, bufferSize) < 0) {
            LOG(SoftISPPipeline, Error) << "ftruncate failed for buffer " << i;
            close(fd);
            return -errno;
        }
        
        // Create a plane
        FrameBuffer::Plane plane;
        plane.fd = SharedFD(fd);  // Wrap FD in SharedFD
        plane.offset = 0;
        plane.length = bufferSize;
        
        // Create buffer with the plane
        std::vector<FrameBuffer::Plane> planes;
        planes.push_back(std::move(plane));
        
        auto buffer = std::make_unique<FrameBuffer>(
            Span<const FrameBuffer::Plane>(planes),
            static_cast<unsigned int>(i)
        );
        
        // Store buffer ID for retrieval
        uint32_t bufferId = static_cast<uint32_t>(buffers->size());
        storeBuffer(bufferId, buffer.get());
        
        buffers->push_back(std::move(buffer));
        
        LOG(SoftISPPipeline, Debug) << "Created buffer " << i << " (FD: " << fd << ", Size: " << bufferSize << ")";
    }
    
    LOG(SoftISPPipeline, Info) << "Successfully exported " << buffers->size() << " frame buffers";
    return 0;
}
