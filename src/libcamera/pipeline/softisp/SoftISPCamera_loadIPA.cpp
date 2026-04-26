/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <libcamera/ipa/core_ipa_interface.h>
#include <libcamera/controls.h>

int SoftISPCameraData::loadIPA()
{
	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA() started";

	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(pipe(), 0, 0);
	LOG(SoftISPPipeline, Info) << "[PIPE] createIPA returned: " << (ipa_ ? "ok" : "null");

	if (!ipa_) {
		LOG(SoftISPPipeline, Warning) << "Failed to create IPA, running without IPA";
		return 0;
	}

	LOG(SoftISPPipeline, Info) << "[PIPE] connecting signals...";
	ipa_->metadataReady.connect(this, &SoftISPCameraData::metadataReady);
	ipa_->frameDone.connect(this, &SoftISPCameraData::frameDone);

	LOG(SoftISPPipeline, Info) << "[PIPE] Calling init()...";
	IPASettings settings;
	ControlInfoMap ctrlInfoMap;
	ControlInfoMap *ipaCtrls = &ctrlInfoMap;
	bool ccmEnabled = false;
	IPACameraSensorInfo sensorInfo = {};
	SharedFD statsFd, paramsFd;

	int ret = ipa_->init(settings, statsFd, paramsFd,
			    sensorInfo, ctrlInfoMap, ipaCtrls, &ccmEnabled);
	LOG(SoftISPPipeline, Info) << "[PIPE] init() returned: " << ret;

	if (ret < 0) {
		ipa_.reset();
		return 0;
	}

	LOG(SoftISPPipeline, Info) << "[PIPE] Calling start()...";
	ret = ipa_->start();
	LOG(SoftISPPipeline, Info) << "[PIPE] start() returned: " << ret;

	if (ret < 0) {
		ipa_.reset();
		return 0;
	}

	LOG(SoftISPPipeline, Info) << "[PIPE] IPA loaded and ready";
	return 0;
}