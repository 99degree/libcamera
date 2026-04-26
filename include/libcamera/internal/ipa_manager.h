/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * Image Processing Algorithm module manager
 */

#pragma once

#include <memory>
#include <stdint.h>
#include <vector>

#include <libcamera/base/log.h>

#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/ipa/ipa_module_info.h>

#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/global_configuration.h"
#include "libcamera/internal/ipa_module.h"
#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/pub_key.h"

namespace libcamera {

LOG_DECLARE_CATEGORY(IPAManager)

class IPAManager
{
public:
	IPAManager(const GlobalConfiguration &configuration);
	~IPAManager();

	template<typename T>
	static std::unique_ptr<T> createIPA(PipelineHandler *pipe,
					    uint32_t minVersion,
					    uint32_t maxVersion)
	{
		CameraManager *cm = pipe->cameraManager();
		IPAManager *self = cm->_d()->ipaManager();
		IPAModule *m = self->module(pipe, minVersion, maxVersion);
		if (!m)
			return nullptr;

		const GlobalConfiguration &configuration = cm->_d()->configuration();

		LOG(IPAManager, Info) << "[IPA_MGR] createIPA: found module, creating proxy...";
		auto proxy = [&]() -> std::unique_ptr<T> {
			char *noIso = utils::secure_getenv("LIBCAMERA_IPA_NO_ISOLATION");
			bool forceNoIsolation = noIso && noIso[0] != '\0';

			if (forceNoIsolation || self->isSignatureValid(m)) {
				LOG(IPAManager, Info) << "[IPA_MGR] createIPA: using Threaded";
				return std::make_unique<typename T::Threaded>(m, configuration);
			} else {
				LOG(IPAManager, Info) << "[IPA_MGR] createIPA: using Isolated";
				return std::make_unique<typename T::Isolated>(m, configuration);
			}
		}();

		LOG(IPAManager, Info) << "[IPA_MGR] createIPA: proxy created, checking valid...";
		if (!proxy->isValid()) {
			LOG(IPAManager, Error) << "[IPA_MGR] createIPA: proxy not valid";
			return nullptr;
		}

		LOG(IPAManager, Info) << "[IPA_MGR] createIPA: proxy valid, returning";
		return proxy;
	}

#if HAVE_IPA_PUBKEY
	static const PubKey &pubKey()
	{
		return pubKey_;
	}
#endif

private:
	void parseDir(const char *libDir, unsigned int maxDepth,
		      std::vector<std::string> &files);
	unsigned int addDir(const char *libDir, unsigned int maxDepth = 0);

	IPAModule *module(PipelineHandler *pipe, uint32_t minVersion,
			  uint32_t maxVersion);

	bool isSignatureValid(IPAModule *ipa) const;

	std::vector<std::unique_ptr<IPAModule>> modules_;

#if HAVE_IPA_PUBKEY
	static const uint8_t publicKeyData_[];
	static const PubKey pubKey_;
	bool forceIsolation_;
#endif
};

} /* namespace libcamera */
