/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2021, The LegacyClonk Team and contributors
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

// Interfaces for music and sound playback handling

#pragma once

#include "Standard.h"

#include <cstddef>
#include <cstdint>

class C4AudioSystem
{
protected:
	C4AudioSystem() = default;

public:
	C4AudioSystem(const C4AudioSystem &) = delete;
	C4AudioSystem(C4AudioSystem &&) = delete;
	virtual ~C4AudioSystem() = default;
	C4AudioSystem &operator=(const C4AudioSystem &) = delete;

	static C4AudioSystem *NewInstance(int maxChannels);

	class MusicFile;
	class SoundFile;

	virtual void FadeOutMusic(std::int32_t ms) = 0;
	virtual bool IsMusicPlaying() const = 0;
	// Plays music file. If supported by audio library, playback starts paused.
	virtual void PlayMusic(const MusicFile *const music, bool loop) = 0;
	// Range: 0.0f through 1.0f
	virtual void SetMusicVolume(float volume) = 0;
	virtual void StopMusic() = 0;
	virtual void UnpauseMusic() = 0;

	class MusicFile
	{
	protected:
		MusicFile() = default;

	public:
		//MusicFile(const void *buf, std::size_t size);
		MusicFile(const MusicFile &) = delete;
		MusicFile(MusicFile &&) = default;
		virtual ~MusicFile() = default;
		MusicFile &operator=(const MusicFile &) = delete;
	};

	virtual MusicFile *CreateMusicFile(const void *buf, std::size_t size) = 0;

	class SoundChannel
	{
	protected:
		SoundChannel() = default;

	public:
		/*// Plays sound file. If supported by audio library, playback starts paused.
		SoundChannel(const SoundFile &sound, bool loop);*/
		SoundChannel(const SoundChannel &) = delete;
		SoundChannel(SoundChannel &&) = delete;
		virtual ~SoundChannel() = default;
		SoundChannel &operator=(const SoundChannel &) = delete;

		virtual bool IsPlaying() const = 0;
		virtual void SetPosition(std::uint32_t ms) = 0;
		// volume range: 0.0f through 1.0f; pan range: -1.0f through 1.0f
		virtual void SetVolumeAndPan(float volume, float pan) = 0;
		virtual void Unpause() = 0;
	};

	virtual SoundChannel *CreateSoundChannel(const SoundFile *const sound, bool loop) = 0;

	class SoundFile
	{
	protected:
		SoundFile() = default;

	public:
		//SoundFile(const void *buf, std::size_t size);
		SoundFile(const SoundFile &) = delete;
		SoundFile(SoundFile &&) = delete;
		virtual ~SoundFile() = default;
		SoundFile &operator=(const SoundFile &) = delete;

		virtual std::uint32_t GetDuration() const = 0;
	};

	virtual SoundFile *CreateSoundFile(const void *buf, std::size_t size) = 0;

public:
	static constexpr auto MaxChannels = 1024;
};
