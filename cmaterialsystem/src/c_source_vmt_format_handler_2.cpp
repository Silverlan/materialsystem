/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "matsysdefinitions.h"
#include "c_source_vmt_format_handler.hpp"
#include "cmaterial_manager2.hpp"
#include "shaders/c_shader_decompose_cornea.hpp"
#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include "shaders/c_shader_extract_image_channel.hpp"
#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include "shaders/source2/c_shader_decompose_pbr.hpp"
#include "texturemanager/texture.h"
#include "sprite_sheet_animation.hpp"
#include "texturemanager/texture_manager2.hpp"
#include <prosper_util.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <prosper_command_buffer.hpp>
#include <textureinfo.h>
#include <image/prosper_texture.hpp>
#include <sharedutils/util_string.h>
#include <sharedutils/util_path.hpp>
#include <sharedutils/datastream.h>
#include <util_texture_info.hpp>
#include <datasystem.h>
#include <datasystem_vector.h>
#include <fsys/ifile.hpp>

#ifndef DISABLE_VMT_SUPPORT
#ifdef ENABLE_VKV_PARSER
#include "util_vmt.hpp"
#include <VKVParser/library.h>
msys::CSourceVmtFormatHandler2::CSourceVmtFormatHandler2(util::IAssetManager &assetManager) : SourceVmtFormatHandler2 {assetManager} {}
bool msys::CSourceVmtFormatHandler2::LoadVmtData(const std::string &vmtShader, ds::Block &rootData, std::string &matShader)
{
	auto r = SourceVmtFormatHandler2::LoadVmtData(vmtShader, rootData, matShader);
	if(!r)
		return false;
	return load_vmt_data(*this, vmtShader, rootData, matShader);
}
#endif
#endif
