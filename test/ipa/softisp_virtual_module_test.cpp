/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024
 *
 * Test loading of the SoftISP Virtual IPA module and verify its info
 */

#include <iostream>
#include <string.h>

#include "libcamera/internal/ipa_module.h"
#include "test.h"

using namespace std;
using namespace libcamera;

class SoftISPVirtualModuleTest : public Test
{
protected:
	int runTest(const string &path, const struct IPAModuleInfo &testInfo)
	{
		int ret = 0;
		IPAModule *ll = new IPAModule(path);

		if (!ll->isValid()) {
			cerr << "SoftISP Virtual IPA module " << path << " is invalid" << endl;
			delete ll;
			return -1;
		}

		const struct IPAModuleInfo &info = ll->info();

		if (memcmp(&info, &testInfo, sizeof(info))) {
			cerr << "SoftISP Virtual IPA module information mismatch:" << endl;
			cerr << "Expected:" << endl;
			cerr << "  moduleAPIVersion = " << testInfo.moduleAPIVersion << endl;
			cerr << "  pipelineVersion = " << testInfo.pipelineVersion << endl;
			cerr << "  pipelineName = " << testInfo.pipelineName << endl;
			cerr << "  name = " << testInfo.name << endl;
			cerr << "Got:" << endl;
			cerr << "  moduleAPIVersion = " << info.moduleAPIVersion << endl;
			cerr << "  pipelineVersion = " << info.pipelineVersion << endl;
			cerr << "  pipelineName = " << info.pipelineName << endl;
			cerr << "  name = " << info.name << endl;
			ret = -1;
		} else {
			cout << "SoftISP Virtual IPA module loaded successfully:" << endl;
			cout << "  name = " << info.name << endl;
			cout << "  pipelineName = " << info.pipelineName << endl;
		}

		delete ll;
		return ret;
	}

	int run() override
	{
		int count = 0;

		/* Test the virtual-softisp module */
		const struct IPAModuleInfo testInfo = {
			IPA_MODULE_API_VERSION,
			0,
			"softisp-virtual",
			"virtual-softisp",
		};

		count += runTest("src/ipa/softisp/ipa_softisp_virtual.so", testInfo);

		if (count < 0)
			return TestFail;

		return TestPass;
	}
};

TEST_REGISTER(SoftISPVirtualModuleTest)
