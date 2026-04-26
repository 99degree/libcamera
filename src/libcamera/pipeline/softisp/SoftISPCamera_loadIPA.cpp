/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <libcamera/ipa/core_ipa_interface.h>
#include <libcamera/controls.h>

int SoftISPCameraData::loadIPA()
{
	LOG(SoftISPPipeline, Info) << "[PIPE] loadIPA() started";

	try {
		ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(pipe(), 0, 0);
	} catch (const std::exception &e) {
		LOG(SoftISPPipeline, Warning) << "IPA creation failed: " << e.what();
		return 0;
	} catch (...) {
		LOG(SoftISPPipeline, Warning) << "IPA creation crashed, running without IPA";
		return 0;
	}

	LOG(SoftISPPipeline, Info) << "[PIPE] createIPA returned: " << (ipa_ ? "ok" : "null");
	if (!ipa_) return 0;

	ipa_->metadataReady.connect(this, &SoftISPCameraData::metadataReady);
	ipa_->frameDone.connect(this, &SoftISPCameraData::frameDone);

	IPASettings settings;
	ControlInfoMap ctrlInfoMap;
	ControlInfoMap *ipaCtrls = &ctrlInfoMap;
	bool ccmEnabled = false;
	IPACameraSensorInfo sensorInfo = {};
	SharedFD statsFd, paramsFd;

	int ret = ipa_->init(settings, statsFd, paramsFd,
			    sensorInfo, ctrlInfoMap, ipaCtrls, &ccmEnabled);
	if (ret < 0) { ipa_.reset(); return 0; }

	ret = ipa_->start();
	if (ret < 0) { ipa_.reset(); return 0; }

	LOG(SoftISPPipeline, Info) << "[PIPE] IPA loaded and ready";
	return 0;
}