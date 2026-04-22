/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * SoftISP Test Application
 * Tests the SoftISP pipeline with synthetic Bayer patterns
 */

#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include <libcamera/base/log.h>
#include <libcamera/base/shared_fd.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>


using namespace libcamera;

LOG_DEFINE_CATEGORY(SoftIspTest)

/* -----------------------------------------------------------------
 * Helper: Generate Bayer Pattern
 * ----------------------------------------------------------------- */
/* 
 * Frame Pool Concept: Pre-generate frames with diverse characteristics
 * Each frame has unique white balance, brightness, noise, and scene properties
 */
struct FrameCharacteristics {
	float rGain;      /* Red white balance gain */
	float gGain;      /* Green white balance gain */
	float bGain;      /* Blue white balance gain */
	float brightness; /* Base brightness level */
	float noiseStd;   /* Noise standard deviation */
	float vignette;   /* Vignette strength */
	float contrast;   /* Contrast factor */
};

static std::vector<FrameCharacteristics> g_framePool;
static bool g_poolInitialized = false;
static std::random_device g_rd;
static std::mt19937 g_gen;

static void initFramePool(uint32_t numFrames)
{
	if (g_poolInitialized) return;
	
	g_gen.seed(g_rd());
	g_framePool.resize(numFrames);
	
	std::uniform_real_distribution<> wbDist(1.2f, 2.5f);      /* WB gain range */
	std::uniform_real_distribution<> brightDist(100.0f, 160.0f); /* Brightness range */
	std::uniform_real_distribution<> noiseDist(5.0f, 15.0f);   /* Noise range */
	std::uniform_real_distribution<> vignetteDist(0.1f, 0.4f); /* Vignette range */
	std::uniform_real_distribution<> contrastDist(0.8f, 1.3f); /* Contrast range */
	
	std::cout << "Initializing frame pool with " << numFrames << " diverse frames..." << std::endl;
	
	for (uint32_t i = 0; i < numFrames; ++i) {
		/* Simulate different lighting conditions */
		float wbTemp = static_cast<float>(i) / numFrames; /* Progress through conditions */
		
		/* White balance: vary from cool to warm */
		if (wbTemp < 0.33f) {
			/* Cool light (blue-ish, ~7000K) */
			g_framePool[i].rGain = wbDist(gen) * 0.8f;
			g_framePool[i].gGain = 1.0f;
			g_framePool[i].bGain = wbDist(gen) * 1.3f;
		} else if (wbTemp < 0.66f) {
			/* Neutral light (~5500K) */
			g_framePool[i].rGain = wbDist(gen);
			g_framePool[i].gGain = 1.0f;
			g_framePool[i].bGain = wbDist(gen);
		} else {
			/* Warm light (red-ish, ~3000K) */
			g_framePool[i].rGain = wbDist(gen) * 1.4f;
			g_framePool[i].gGain = 1.0f;
			g_framePool[i].bGain = wbDist(gen) * 0.7f;
		}
		
		/* Brightness: vary from dim to bright */
		g_framePool[i].brightness = brightDist(g_gen);
		
		/* Noise: vary from low to high ISO */
		g_framePool[i].noiseStd = noiseDist(g_gen);
		
		/* Vignette: vary lens characteristics */
		g_framePool[i].vignette = vignetteDist(g_gen);
		
		/* Contrast: vary scene contrast */
		g_framePool[i].contrast = contrastDist(g_gen);
		
		if (i < 3 || i == numFrames - 1) {
			std::cout << "  Frame " << i << ": WB(R=" 
					<< g_framePool[i].rGain << ",G=" << g_framePool[i].gGain 
					<< ",B=" << g_framePool[i].bGain 
					<< "), Bright=" << g_framePool[i].brightness 
					<< ", Noise=" << g_framePool[i].noiseStd << std::endl;
		}
	}
	
	g_poolInitialized = true;
	std::cout << "Frame pool initialized successfully!" << std::endl;
}

static void generateBayerPattern(uint8_t *data, uint32_t width, uint32_t height, uint32_t frameId)
{
	/* Ensure frame pool is initialized */
	static uint32_t g_poolSize = 20; /* Pool size */
	if (!g_poolInitialized) {
		initFramePool(g_poolSize);
	}
	
	/* Get characteristics for this frame (cycle through pool) */
	const FrameCharacteristics &chars = g_framePool[frameId % g_framePool.size()];
	
	/* Pre-compute noise for this frame (unique per frame) */
	static std::vector<std::vector<float>> g_frameNoise;
	if (g_frameNoise.empty()) {
		g_frameNoise.resize(g_framePool.size());
		std::normal_distribution<> noiseGen(0.0, 1.0);
		for (size_t f = 0; f < g_framePool.size(); ++f) {
			g_frameNoise[f].resize(width * height);
			for (size_t i = 0; i < g_frameNoise[f].size(); ++i) {
				g_frameNoise[f][i] = noiseGen(g_gen);
			}
		}
	}
	
	/* 
	 * Generate Bayer GRBG pattern with frame-specific characteristics:
	 * - Unique white balance per frame
	 * - Varying brightness and contrast
	 * - Frame-specific noise pattern
	 * - Different vignette strengths
	 */
	
	for (uint32_t y = 0; y < height; ++y) {
		/* Vignetting factor (darker at edges) - frame specific */
		const float vignetteY = 1.0f - chars.vignette * std::abs(2.0f * y / height - 1.0f);
		
		for (uint32_t x = 0; x < width; ++x) {
			uint32_t idx = (y * width + x);
			
			/* Vignetting factor for x */
			const float vignetteX = 1.0f - chars.vignette * std::abs(2.0f * x / width - 1.0f);
			const float vignette = vignetteX * vignetteY;
			
			/* Bayer GRBG pattern with frame-specific white balance */
			const bool rowEven = (y % 2 == 0);
			const bool colEven = (x % 2 == 0);
			
			float baseValue;
			if (rowEven) {
				if (colEven) {
					/* Green pixel (top row) */
					baseValue = chars.brightness * chars.gGain;
				} else {
					/* Red pixel */
					baseValue = chars.brightness * chars.rGain;
				}
			} else {
				if (colEven) {
					/* Blue pixel */
					baseValue = chars.brightness * chars.bGain;
				} else {
					/* Green pixel (bottom row) */
					baseValue = chars.brightness * chars.gGain;
				}
			}
			
			/* Apply vignetting */
			baseValue *= vignette;
			
			/* Apply contrast */
			baseValue = 128.0f + (baseValue - 128.0f) * chars.contrast;
			
			/* Add spatial variation (simulating texture/pattern) */
			const float spatialVar = 8.0f * std::sin(x * 0.1f) * std::sin(y * 0.1f);
			baseValue += spatialVar;
			
			/* Add frame-specific noise (scaled by frame's noise level) */
			baseValue += g_frameNoise[frameId % g_framePool.size()][idx] * chars.noiseStd;
			
			/* Clamp to valid range [0, 255] */
			data[idx] = static_cast<uint8_t>(std::clamp(baseValue, 0.0f, 255.0f));
		}
	}
}

/* -----------------------------------------------------------------
 * Helper: Save Frame to PGM file (raw 8-bit)
 * ----------------------------------------------------------------- */
static bool saveFrame(const FrameBuffer *buffer, const std::string &filename, uint32_t width, uint32_t height)
{
	int fd = buffer->planes()[0].fd.get();
	if (fd < 0) {
		std::cerr << "Invalid fd" << std::endl;
		return false;
	}

	void *memory = mmap(nullptr, buffer->planes()[0].memory.size, PROT_READ, MAP_SHARED, fd, 0);
	if (memory == MAP_FAILED) {
		std::cerr << "Failed to mmap buffer" << std::endl;
		return false;
	}

	/* Write PGM header */
	std::ofstream out(filename, std::ios::binary);
	if (!out) {
		std::cerr << "Failed to open file: " << filename << std::endl;
		munmap(memory, buffer->planes()[0].memory.size);
		return false;
	}

	out << "P5\n" << width << " " << height << "\n255\n";
	out.write(static_cast<char*>(memory), width * height);

	munmap(memory, buffer->planes()[0].memory.size);
	std::cout << "Saved frame to " << filename << std::endl;
	return true;
}

/* -----------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------- */
int main(int argc, char *argv[])
{
	int frames = 5;
	std::string pipeline = "dummysoftisp";
	bool saveFrames = false;
	
	static struct option long_options[] = {
		{ "frames", required_argument, 0, 'f' },
		{ "pipeline", required_argument, 0, 'p' },
		{ "save", no_argument, 0, 's' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "f:p:sh", long_options, nullptr)) != -1) {
		switch (opt) {
		case 'f':
			frames = std::stoi(optarg);
			break;
		case 'p':
			pipeline = optarg;
			break;
		case 's':
			saveFrames = true;
			break;
		case 'h':
		default:
			std::cout << "Usage: " << argv[0] << " [options]\n"
			          << "Options:\n"
			          << "  -f, --frames N   Number of frames (default: 5)\n"
			          << "  -p, --pipeline   Pipeline name (default: dummysoftisp)\n"
			          << "  -s, --save       Save frames to disk as PGM\n"
			          << "  -h, --help       Show this help\n";
			return 0;
		}
	}

	/* Initialize CameraManager */
	CameraManager cm;
	if (cm.start() < 0) {
		std::cerr << "Failed to start CameraManager" << std::endl;
		return -1;
	}

	/* Enumerate cameras */
	auto cameras = cm.cameras();
	if (cameras.empty()) {
		std::cerr << "No cameras found" << std::endl;
		cm.stop();
		return -1;
	}

	/* Find the requested pipeline */
	std::shared_ptr<Camera> camera;
	for (auto &cam : cameras) {
		if (cam->id().find(pipeline) != std::string::npos) {
			camera = cam;
			break;
		}
	}

	if (!camera) {
		std::cerr << "Camera not found: " << pipeline << std::endl;
		cm.stop();
		return -1;
	}

	std::cout << "Using camera: " << camera->id() << std::endl;

	/* Acquire camera */
	if (cm.acquire(camera) < 0) {
		std::cerr << "Failed to acquire camera" << std::endl;
		cm.stop();
		return -1;
	}

	/* Generate configuration (FHD default) */
	auto config = camera->generateConfiguration({ StreamRole::Raw });
	if (!config) {
		std::cerr << "Failed to generate configuration" << std::endl;
		return -1;
	}

	/* Validate and update configuration */
	if (config->validate() == CameraConfiguration::Invalid) {
		std::cerr << "Invalid configuration" << std::endl;
		return -1;
	}

	/* Configure camera */
	if (camera->configure(config.get()) < 0) {
		std::cerr << "Failed to configure camera" << std::endl;
		return -1;
	}

	/* Get stream info */
	Stream *stream = config->at(0).stream();
	if (!stream) {
		std::cerr << "No stream found" << std::endl;
		return -1;
	}

	uint32_t width = stream->configuration().size.width;
	uint32_t height = stream->configuration().size.height;
	std::cout << "Stream configured: " << width << "x" << height 
	          << " (Format: " << stream->configuration().pixelFormat << ")" << std::endl;

	/* Allocate buffers */
	FrameBufferAllocator allocator(camera);
	std::vector<std::unique_ptr<FrameBuffer>> buffers;
	for (unsigned int i = 0; i < config->at(0).bufferCount; ++i) {
		std::unique_ptr<FrameBuffer> buffer;
		int ret = allocator.allocate(stream, buffer);
		if (ret < 0) {
			std::cerr << "Failed to allocate buffer" << std::endl;
			return -1;
		}
		buffers.push_back(std::move(buffer));
	}

	/* Create requests */
	std::vector<std::unique_ptr<Request>> requests;
	for (auto &buffer : buffers) {
		std::unique_ptr<Request> request = camera->createRequest();
		if (!request) {
			std::cerr << "Failed to create request" << std::endl;
			return -1;
		}
		
		/* Attach buffer */
		if (request->addBuffer(stream, buffer.get()) < 0) {
			std::cerr << "Failed to add buffer to request" << std::endl;
			return -1;
		}
		
		requests.push_back(std::move(request));
	}

	/* Start camera */
	if (camera->start() < 0) {
		std::cerr << "Failed to start camera" << std::endl;
		return -1;
	}

	std::cout << "Camera started. Processing " << frames << " frames..." << std::endl;

	/* Process frames */
	for (int i = 0; i < frames; ++i) {
		Request &request = *requests[i % requests.size()];
		
		/* Generate Bayer pattern in the buffer */
		FrameBuffer *buffer = request.buffers()[stream];
		if (buffer) {
			int fd = buffer->planes()[0].fd.get();
			void *memory = mmap(nullptr, buffer->planes()[0].memory.size, 
			                    PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
			if (memory != MAP_FAILED) {
				generateBayerPattern(static_cast<uint8_t*>(memory), width, height, i);
				munmap(memory, buffer->planes()[0].memory.size);
			}
		}

		/* Queue request */
		if (request.queue() < 0) {
			std::cerr << "Failed to queue request" << std::endl;
			break;
		}

		std::cout << "Frame " << (i + 1) << "/" << frames << " queued" << std::endl;

		/* Wait for completion (simple sleep for now) */
		usleep(100000); // 100ms

		/* Save frame if requested */
		if (saveFrames) {
			std::string filename = "frame_" + std::to_string(i) + ".pgm";
			saveFrame(buffer, filename, width, height);
		}
	}

	/* Stop camera */
	camera->stop();

	/* Cleanup */
	cm.release(camera);
	cm.stop();

	std::cout << "Test completed successfully!" << std::endl;
	return 0;
}
