/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __LOADIMAGEDATA_H__
#define __LOADIMAGEDATA_H__

#include "cmatsysdefinitions.h"

struct PNGInfo;
class TextureQueueItem;
bool LoadPNGData(const char *path,PNGInfo &info);
void InitializeTextureData(TextureQueueItem *item);

#endif