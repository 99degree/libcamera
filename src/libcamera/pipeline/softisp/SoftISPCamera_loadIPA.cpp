/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::loadIPA()
{
	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: begin";

	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: calling createIPA...";
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(pipe(), 0, 0);
	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: createIPA returned";

	if (!ipa_) {
		LOG(SoftISPPipeline, Warning) << "[PIPE] loadIPA: ipa_ is null";
		return 0;
	}

	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: checking isValid()...";
	bool valid = ipa_->isValid();
	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: isValid() = " << (valid ? "true" : "false");

	if (valid) {
		LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: connecting metadataReady...";
		ipa_->metadataReady.connect(this, &SoftISPCameraData::metadataReady);
		LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: connecting frameDone...";
		ipa_->frameDone.connect(this, &SoftISPCameraData::frameDone);
	}

	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA: done";
	return 0;
}