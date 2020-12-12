/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2020, The LegacyClonk Team and contributors
 *
 * Distributed under the terms of the ISC license; see accompanying file
 * "COPYING" for details.
 *
 * "Clonk" is a registered trademark of Matthes Bender, used with permission.
 * See accompanying file "TRADEMARK" for details.
 *
 * To redistribute this file separately, substitute the full license texts
 * for the above references.
 */

#pragma once

#include "C4AudioSystem.h"

class C4AudioSystemNone : public C4AudioSystem
{
public:
	C4AudioSystemNone() = default;
	void FadeOutMusic(std::int32_t) override {}
	bool IsMusicPlaying() const override { return false; }
	void PlayMusic(const MusicFile *const, bool) override {}
	void SetMusicVolume(float) override {}
	void StopMusic() override {}
	void UnpauseMusic() override {}

	class MusicFileNone : public MusicFile
	{
	public:
		MusicFileNone() = default;
	};

	MusicFile *CreateMusicFile(const void *, std::size_t) override { return new MusicFileNone{}; }

	class SoundChannelNone : public SoundChannel
	{
	public:
		SoundChannelNone() = default;
		bool IsPlaying() const override { return false; }
		void SetPosition(std::uint32_t) override {}
		void SetVolumeAndPan(float, float) override {}
		void Unpause() override {}
	};

	SoundChannel *CreateSoundChannel(const SoundFile *const, bool) override { return new SoundChannelNone{}; }

	class SoundFileNone : public SoundFile
	{
	public:
		SoundFileNone() = default;
		std::uint32_t GetDuration() const override { return 0u; }
	};

	SoundFile *CreateSoundFile(const void *, std::size_t) override { return new SoundFileNone{}; }
};
