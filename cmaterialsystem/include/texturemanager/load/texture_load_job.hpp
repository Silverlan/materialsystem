/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_LOAD_JOB_HPP__
#define __MSYS_TEXTURE_LOAD_JOB_HPP__

#include "cmatsysdefinitions.h"
#include <cinttypes>
#include <string>
#include <memory>
#include <chrono>

namespace msys
{
	using TextureLoadJobId = uint64_t;
	using TextureLoadJobPriority = int32_t;
	class ITextureFormatHandler;
	class TextureProcessor;
	struct DLLCMATSYS TextureLoadJob
	{
		enum class State : uint8_t
		{
			Pending = 0,
			Succeeded,
			Failed
		};
		TextureLoadJob()=default;
		TextureLoadJob(TextureLoadJob &&other)=default;
		TextureLoadJob(const TextureLoadJob &other)=default;
		TextureLoadJob &operator=(const TextureLoadJob&)=default;
		TextureLoadJob &operator=(TextureLoadJob&&)=default;

		std::shared_ptr<ITextureFormatHandler> handler;
		std::shared_ptr<TextureProcessor> processor = nullptr;
		State state = State::Pending;
		TextureLoadJobPriority priority = 0;
		std::string textureIdentifier;
		TextureLoadJobId jobId = 0;

		std::chrono::high_resolution_clock::time_point queueStartTime;
		std::chrono::high_resolution_clock::time_point completionTime;
		std::chrono::high_resolution_clock::time_point taskStartTime;
	};
	class CompareTextureLoadJob
	{
	public:
		bool operator()(TextureLoadJob& t1, TextureLoadJob& t2)
		{
			return t1.priority > t2.priority;
		}
	};
};

#endif
