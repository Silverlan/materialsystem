/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/texture_format_handler.hpp"

msys::ITextureFormatHandler::ITextureFormatHandler(util::IAssetManager &assetManager) : util::IAssetFormatHandler {assetManager} {}

bool msys::ITextureFormatHandler::LoadData() { return LoadData(m_inputTextureInfo); }
