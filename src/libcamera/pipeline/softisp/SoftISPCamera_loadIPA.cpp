/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::loadIPA()
{
	/*
	 * IPA loading is unstable due to ORT initialization in Threaded proxy.
	 * Set LIBCAMERA_SOFTISP_IPA=1 to enable (also requires
	 * LIBCAMERA_IPA_NO_ISOLATION=1 and SOFTISP_MODEL_DIR).
	 */
	char *enable = getenv("LIBCAMERA_SOFTISP_IPA");
	if (!enable || enable[0] == '\0') {
		LOG(SoftISPPipeline, Info) << "IPA loading skipped (set LIBCAMERA_SOFTISP_IPA=1 to enable)";
		return 0;
	}

	LOG(SoftISPPipeline, Info) << "Loading SoftISP IPA";

	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPPipeline, Warning) << "Failed to create SoftISP IPA";
		return 0;
	}

	if (ipa_->isValid()) {
		ipa_->metadataReady.connect(this, &SoftISPCameraData::metadataReady);
		ipa_->frameDone.connect(this, &SoftISPCameraData::frameDone);
		ipa_->start();
		LOG(SoftISPPipeline, Info) << "SoftISP IPA loaded and started";
	}

	return 0;
}