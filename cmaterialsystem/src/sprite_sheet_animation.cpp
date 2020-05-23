/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "sprite_sheet_animation.hpp"
#include <fsys/filesystem.h>

static std::array<char,3> PSD_HEADER {'P','S','D'};
constexpr uint32_t PSD_VERSION = 0;

#pragma optimize("",off)
void SpriteSheetAnimation::Save(std::shared_ptr<VFilePtrInternalReal> &f) const
{
	f->Write(PSD_HEADER.data(),PSD_HEADER.size());
	f->Write<uint32_t>(PSD_VERSION);

	f->Write<uint32_t>(sequences.size());
	for(auto &seq : sequences)
	{
		f->Write<bool>(seq.loop);
		auto &frames = seq.frames;
		auto numFrames = frames.size();
		f->Write<uint32_t>(numFrames);
		for(auto &frame : frames)
		{
			f->Write<Vector2>(frame.uvStart);
			f->Write<Vector2>(frame.uvEnd);
			f->Write<float>(frame.duration);
		}
	}
}
bool SpriteSheetAnimation::Load(std::shared_ptr<VFilePtrInternal> &f)
{
	auto header = f->Read<std::array<char,3>>();
	if(header != PSD_HEADER)
		return false;
	auto version = f->Read<uint32_t>();
	if(version != 0)
		return false;
	auto numSequences = f->Read<uint32_t>();
	sequences.resize(numSequences);
	for(auto &seq : sequences)
	{
		seq.loop = f->Read<bool>();
		auto numFrames = f->Read<uint32_t>();
		auto &frames = seq.frames;
		frames.resize(numFrames);
		for(auto &frame : frames)
		{
			frame.uvStart = f->Read<Vector2>();
			frame.uvEnd = f->Read<Vector2>();
			frame.duration = f->Read<float>();
		}
	}
	return true;
}
bool SpriteSheetAnimation::Sequence::GetInterpolatedFrameData(float ptTime,uint32_t &outFrame0,uint32_t &outFrame1,float &outInterpFactor) const
{
	if(frames.empty())
		return false;
	float time = 0.f;
	for(auto iFrame=decltype(frames.size()){0u};iFrame<frames.size();++iFrame)
	{
		auto &frame = frames.at(iFrame);
		auto frameDuration = frame.duration;
		if(ptTime >= time && ptTime < (time +frameDuration))
		{
			auto dtTimeRelativeToFrame = ptTime -time;
			outInterpFactor = dtTimeRelativeToFrame /frameDuration;
			outFrame0 = iFrame;
			outFrame1 = iFrame +1;
			if(outFrame1 >= frames.size())
			{
				if(loop == false)
				{
					outFrame1 = outFrame0;
					outInterpFactor = 0.f;
					break;
				}
				outFrame1 = 0;
			}
			break;
		}
		time += frameDuration;
	}
	return true;
}
#pragma optimize("",on)
