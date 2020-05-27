/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright (c) 2020 Florian Weischer
*/

#ifndef __SPRITE_SHEET_ANIMATION_HPP__
#define __SPRITE_SHEET_ANIMATION_HPP__

#include "cmatsysdefinitions.h"
#include <mathutil/uvec.h>
#include <vector>

class VFilePtrInternal;
class VFilePtrInternalReal;
struct DLLCMATSYS SpriteSheetAnimation
{
#if 0
	static constexpr auto SAMPLE_COUNT = 1'024;
	struct DLLCMATSYS SampleData
	{
		Vector4 uvFrame0 {};
		Vector4 uvFrame1 {};
		float interpFactor = 0.f;
	};
	std::array<SampleData,SAMPLE_COUNT> GenerateSamples() const;
#endif
	struct DLLCMATSYS Sequence
	{
		struct DLLCMATSYS Frame
		{
			Vector2 uvStart {};
			Vector2 uvEnd {};

			// Duration of the frame relative to the particle its used for (NOT in seconds!)
			// i.e. duration of 1 is equal to particle time
			float duration = 0.f;
		};
		// If set to false, the animation will end at the last frame, otherwise
		// it will wrap around back to the first frame.
		bool loop = false;
		std::vector<Frame> frames {};

		bool GetInterpolatedFrameData(float ptTime,uint32_t &outFrame0,uint32_t &outFrame1,float &outInterpFactor) const;
		uint32_t GetAbsoluteFrameIndex(uint32_t localFrameIdx) const;
		uint32_t GetLocalFrameIndex(uint32_t absFrameIdx) const;

		uint32_t GetFrameOffset() const;
		float GetDuration() const;
	private:
		friend SpriteSheetAnimation;
		void SetFrameOffset(uint32_t offset);
		uint32_t m_frameOffset = 0;
		float m_duration = 0.f;
	};
	std::vector<Sequence> sequences {};

	uint32_t GetAbsoluteFrameIndex(uint32_t sequenceIdx,uint32_t localFrameIdx) const;
	void Save(std::shared_ptr<VFilePtrInternalReal> &f) const;
	bool Load(std::shared_ptr<VFilePtrInternal> &f);

	void UpdateLookupData();
};

#endif
