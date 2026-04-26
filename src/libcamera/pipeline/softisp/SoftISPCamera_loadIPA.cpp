/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::loadIPA()
{
	LOG(SoftISPPipeline, Info) << "IPA loading stubbed - running without IPA processing";
	
	// TODO: Implement proper IPA loading
	// For now, just return success to allow the pipeline to work without IPA
	
	return 0;
}
