/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MATERIAL_COPY_HPP__
#define __MATERIAL_COPY_HPP__

#include "matsysdefinitions.h"
#include "material.h"
#include <sharedutils/util_shaderinfo.hpp>

template<class TMaterial>
	TMaterial *Material::Copy() const
{
	Material *r = nullptr;
	if(!IsValid())
		r = GetManager().CreateMaterial("pbr");
	else if(m_shaderInfo.expired() == false)
		r = GetManager().CreateMaterial(m_shaderInfo->GetIdentifier(),std::shared_ptr<ds::Block>(m_data->Copy()));
	else
		r = GetManager().CreateMaterial(*m_shader,std::shared_ptr<ds::Block>(m_data->Copy()));
	if(IsLoaded())
		r->SetLoaded(true);
	r->m_stateFlags = m_stateFlags;
	return static_cast<TMaterial*>(r);
}

#endif
