// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __LOADIMAGEDATA_H__
#define __LOADIMAGEDATA_H__

#include "cmatsysdefinitions.h"

struct PNGInfo;
class TextureQueueItem;
bool LoadPNGData(const char *path, PNGInfo &info);
void InitializeTextureData(TextureQueueItem *item);

#endif
