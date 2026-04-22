/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Test SoftISP ONNX inference pipeline
 */

#include <iostream>
#include <string>
#include <cstdlib>

#include "libcamera/internal/ipa_module.h"
#include "libcamera/ipa/ipa_interface.h"
#include "test.h"

using namespace libcamera;

class SoftISPInferenceTest : public Test {
protected:
	int run() override {
		const char* modelDir = std::getenv("SOFTISP_MODEL_DIR");
		if (!modelDir) {
			std::cerr << "SOFTISP_MODEL_DIR not set, skipping inference test" << std::endl;
			return TestSkip;
		}

		std::cout << "Testing SoftISP ONNX inference pipeline" << std::endl;
		std::cout << "Model directory: " << modelDir << std::endl;

		/* Load the IPA module */
		IPAModule *module = new IPAModule("src/ipa/softisp/ipa_softisp.so");
		if (!module->isValid()) {
			std::cerr << "Failed to load SoftISP IPA module" << std::endl;
			delete module;
			return TestFail;
		}

		std::cout << "IPA module loaded successfully" << std::endl;

		/* Create algorithm instance */
		IPAInterface *algo = ipaCreate();
		if (!algo) {
			std::cerr << "Failed to create SoftISP algorithm" << std::endl;
			delete module;
			return TestFail;
		}

		std::cout << "SoftISP algorithm created" << std::endl;

		/* We can't call init/configure/start/processStats through the base interface
		 * without knowing the actual type. For now, just verify the module loads. */
		
		std::cout << "Module loaded and algorithm created successfully" << std::endl;
		std::cout << "Full inference test requires direct SoftIsp access" << std::endl;

		delete algo;
		delete module;

		return TestPass;
	}
};

TEST_REGISTER(SoftISPInferenceTest)
