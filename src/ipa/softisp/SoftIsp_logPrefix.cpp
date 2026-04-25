/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <sstream>

std::string SoftIsp::logPrefix() const
{
	std::ostringstream oss;
	oss << "SoftIsp@" << this;
	return oss.str();
}
