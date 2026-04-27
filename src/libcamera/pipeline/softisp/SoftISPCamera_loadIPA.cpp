/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::loadIPA()
{
	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA() - creating IPA via IPAManager";

	ipaProxy_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(pipe(), 0, 0);
	if (!ipaProxy_) {
		LOG(SoftISPPipeline, Warning) << "IPA creation failed, running without IPA";
		return 0;
	}

	ipaInterface_ = ipaProxy_.get();
	LOG(SoftISPPipeline, Info) << "[PIPE] IPA created and interface set";

	// Initialize IPA with default settings
	IPASettings settings;
	ControlInfoMap ctrlInfoMap;
	ControlInfoMap *ipaCtrls = &ctrlInfoMap;
	bool ccmEnabled = false;
	IPACameraSensorInfo sensorInfo = {};
	SharedFD statsFd, paramsFd;

	int ret = ipaProxy_->init(settings, statsFd, paramsFd,
				 sensorInfo, ctrlInfoMap, ipaCtrls, &ccmEnabled);
	if (ret < 0) {
		LOG(SoftISPPipeline, Warning) << "IPA init() failed";
		ipaProxy_.reset();
		ipaInterface_ = nullptr;
		return 0;
	}

	ipaProxy_->start();
	LOG(SoftISPPipeline, Info) << "[PIPE] IPA started";

	return 0;
}