// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.cmaterialsystem:texture_manager.load_image_data;

export import pragma.materialsystem;
export import :texture_manager.texture_queue;

export namespace pragma::material {
	bool LoadPNGData(const char *path, PNGInfo &info);
	void InitializeTextureData(TextureQueueItem *item);
}
