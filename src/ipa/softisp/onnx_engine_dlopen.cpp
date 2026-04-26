/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "onnx_engine.h"
#include <dlfcn.h>
#include <libcamera/base/log.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>

namespace libcamera {

LOG_DEFINE_CATEGORY(OnnxEngine)

namespace ipa::soft {

namespace {

using OrtStatusPtr = void *;

using PFN_CreateEnv = OrtStatusPtr (*)(int, int, void *, void **);
using PFN_CreateSessionOptions = OrtStatusPtr (*)(void **);
using PFN_SetIntraOpNumThreads = OrtStatusPtr (*)(void *, int);
using PFN_SetGraphOptLevel = OrtStatusPtr (*)(void *, int);
using PFN_CreateSession = OrtStatusPtr (*)(void *, const char *, void *, void **);
using PFN_CreateMemoryInfo = OrtStatusPtr (*)(int, int, void **);
using PFN_GetInputCount = OrtStatusPtr (*)(const void *, size_t *);
using PFN_GetOutputCount = OrtStatusPtr (*)(const void *, size_t *);
using PFN_GetInputName = OrtStatusPtr (*)(const void *, size_t, void *, char **);
using PFN_GetOutputName = OrtStatusPtr (*)(const void *, size_t, void *, char **);
using PFN_CreateTensor = OrtStatusPtr (*)(void *, void *, size_t, const int64_t *, size_t, int, void **);
using PFN_GetData = OrtStatusPtr (*)(void *, void **);
using PFN_Run = OrtStatusPtr (*)(void *, void *, const char *const *, const void *const *, size_t, const char *const *, size_t, void **);
using PFN_ReleaseEnv = void (*)(void *);
using PFN_ReleaseOptions = void (*)(void *);
using PFN_ReleaseSession = void (*)(void *);
using PFN_ReleaseMemory = void (*)(void *);
using PFN_ReleaseValue = void (*)(void *);
using PFN_CreateAllocator = OrtStatusPtr (*)(const void *, void **);
using PFN_ReleaseAllocator = void (*)(void *);
using PFN_GetErrorMsg = const char *(*)(OrtStatusPtr);
using PFN_ReleaseStatus = void (*)(OrtStatusPtr);

static PFN_CreateEnv            OrtCreateEnv;
static PFN_CreateSessionOptions OrtCreateSessionOptions;
static PFN_SetIntraOpNumThreads OrtSetIntraOpNumThreads;
static PFN_SetGraphOptLevel     OrtSetSessionGraphOptimizationLevel;
static PFN_CreateSession        OrtCreateSession;
static PFN_CreateMemoryInfo     OrtCreateMemoryInfo;
static PFN_GetInputCount        OrtSessionGetInputCount;
static PFN_GetOutputCount       OrtSessionGetOutputCount;
static PFN_GetInputName         OrtSessionGetInputName;
static PFN_GetOutputName        OrtSessionGetOutputName;
static PFN_CreateTensor         OrtCreateTensorAsOrtValue;
static PFN_GetData              OrtGetTensorMutableData;
static PFN_Run                  OrtRun;
static PFN_ReleaseEnv           OrtReleaseEnv;
static PFN_ReleaseOptions       OrtReleaseSessionOptions;
static PFN_ReleaseSession       OrtReleaseSession;
static PFN_ReleaseMemory        OrtReleaseMemoryInfo;
static PFN_ReleaseValue         OrtReleaseValue;
static PFN_CreateAllocator      OrtCreateAllocator;
static PFN_ReleaseAllocator     OrtReleaseAllocator;
static PFN_GetErrorMsg          OrtGetErrorMessage;
static PFN_ReleaseStatus        OrtReleaseStatus;

static bool bindSymbols(void *h)
{
#define BIND(fn, name) fn = (decltype(fn))dlsym(h, name)
	BIND(OrtCreateEnv, "OrtCreateEnv");
	BIND(OrtCreateSessionOptions, "OrtCreateSessionOptions");
	BIND(OrtSetIntraOpNumThreads, "OrtSetIntraOpNumThreads");
	BIND(OrtSetSessionGraphOptimizationLevel, "OrtSetSessionGraphOptimizationLevel");
	BIND(OrtCreateSession, "OrtCreateSession");
	BIND(OrtCreateMemoryInfo, "OrtCreateMemoryInfo");
	BIND(OrtSessionGetInputCount, "OrtSessionGetInputCount");
	BIND(OrtSessionGetOutputCount, "OrtSessionGetOutputCount");
	BIND(OrtSessionGetInputName, "OrtSessionGetInputName");
	BIND(OrtSessionGetOutputName, "OrtSessionGetOutputName");
	BIND(OrtCreateTensorAsOrtValue, "OrtCreateTensorAsOrtValue");
	BIND(OrtGetTensorMutableData, "OrtGetTensorMutableData");
	BIND(OrtRun, "OrtRun");
	BIND(OrtReleaseEnv, "OrtReleaseEnv");
	BIND(OrtReleaseSessionOptions, "OrtReleaseSessionOptions");
	BIND(OrtReleaseSession, "OrtReleaseSession");
	BIND(OrtReleaseMemoryInfo, "OrtReleaseMemoryInfo");
	BIND(OrtReleaseValue, "OrtReleaseValue");
	BIND(OrtCreateAllocator, "OrtCreateAllocator");
	BIND(OrtReleaseAllocator, "OrtReleaseAllocator");
	BIND(OrtGetErrorMessage, "OrtGetErrorMessage");
	BIND(OrtReleaseStatus, "OrtReleaseStatus");
#undef BIND
	return true;
}

} // namespace

OnnxEngineDlopen::OnnxEngineDlopen() = default;

OnnxEngineDlopen::~OnnxEngineDlopen()
{
	if (session_) OrtReleaseSession(session_);
	if (memoryInfo_) OrtReleaseMemoryInfo(memoryInfo_);
	if (options_) OrtReleaseSessionOptions(options_);
	if (env_) OrtReleaseEnv(env_);
	if (ortLib_) dlclose(ortLib_);
}

bool OnnxEngineDlopen::initOrt()
{
	if (ortLib_) return true;

	ortLib_ = dlopen("libonnxruntime.so", RTLD_LAZY | RTLD_GLOBAL);
	if (!ortLib_) {
		LOG(OnnxEngine, Warning) << "dlopen onnxruntime failed";
		return false;
	}

	bindSymbols(ortLib_);

	OrtCreateEnv(2, 0, nullptr, &env_);
	OrtCreateSessionOptions(&options_);
	OrtSetIntraOpNumThreads(options_, 2);
	OrtSetSessionGraphOptimizationLevel(options_, 3);
	OrtCreateMemoryInfo(0, 0, &memoryInfo_);

	return true;
}

int OnnxEngineDlopen::loadModel(const std::string &modelPath)
{
	if (!initOrt()) return -EINVAL;

	if (access(modelPath.c_str(), R_OK) != 0) {
		LOG(OnnxEngine, Warning) << "Model not found: " << modelPath;
		return -ENOENT;
	}

	if (session_) {
		OrtReleaseSession(session_);
		session_ = nullptr;
	}

	OrtCreateSession(env_, modelPath.c_str(), options_, &session_);

	void *alloc = nullptr;
	OrtCreateAllocator(session_, &alloc);

	size_t nIn = 0, nOut = 0;
	OrtSessionGetInputCount(session_, &nIn);
	OrtSessionGetOutputCount(session_, &nOut);

	for (size_t i = 0; i < nIn; i++) {
		char *nm = nullptr;
		OrtSessionGetInputName(session_, i, alloc, &nm);
		inputNames_.push_back(strdup(nm));
	}
	for (size_t i = 0; i < nOut; i++) {
		char *nm = nullptr;
		OrtSessionGetOutputName(session_, i, alloc, &nm);
		outputNames_.push_back(strdup(nm));
	}

	OrtReleaseAllocator(alloc);
	LOG(OnnxEngine, Info) << "Loaded: " << modelPath
			      << " (" << nIn << " in, " << nOut << " out)";
	return 0;
}

int OnnxEngineDlopen::runInference(const std::vector<float> &inputs,
				   std::vector<float> &outputs)
{
	if (!session_) return -EINVAL;

	std::vector<int64_t> shape = {1, (int64_t)inputs.size()};
	void *inTensor = nullptr;
	OrtCreateTensorAsOrtValue(memoryInfo_,
				  (void *)inputs.data(),
				  inputs.size() * sizeof(float),
				  shape.data(), shape.size(),
				  1, &inTensor);

	const char *inNm = inputNames_.empty() ? nullptr : inputNames_[0];
	const char *outNm = outputNames_.empty() ? nullptr : outputNames_[0];
	void *outTensor = nullptr;

	OrtRun(session_, nullptr, &inNm, &inTensor, 1, &outNm, 1, &outTensor);
	OrtReleaseValue(inTensor);

	float *data = nullptr;
	OrtGetTensorMutableData(outTensor, (void **)&data);

	outputs.clear();
	outputs.assign(data, data + inputs.size());

	OrtReleaseValue(outTensor);
	return 0;
}

} // namespace ipa::soft
} // namespace libcamera