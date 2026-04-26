/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::loadIPA()
{
	LOG(SoftISPPipeline, Info) << "IPA loading skipped (ONNX not stable on this platform)";
	return 0;

	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPPipeline, Warning) << "Failed to create SoftISP IPA, running without IPA processing";
		return 0;
	}

	if (ipa_->isValid()) {
		ipa_->metadataReady.connect(this, &SoftISPCameraData::metadataReady);
		ipa_->frameDone.connect(this, &SoftISPCameraData::frameDone);
	}

	LOG(SoftISPPipeline, Info) << "SoftISP IPA loaded successfully";

	return 0;
}