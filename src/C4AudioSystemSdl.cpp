/*
 * LegacyClonk
 *
 * Copyright (c) 2017, The LegacyClonk Team and contributors
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

#include <C4Include.h>
#include <C4AudioSystem.h>

#include <C4Config.h>

#include <Standard.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>

C4AudioSystem::C4AudioSystem()
{
	// Check SDL_mixer version
	SDL_version compile_version;
	MIX_VERSION(&compile_version);
	const auto link_version = Mix_Linked_Version();
	LogF("SDL_mixer runtime version is %d.%d.%d (compiled with %d.%d.%d)",
		link_version->major, link_version->minor, link_version->patch,
		compile_version.major, compile_version.minor, compile_version.patch);

	// Initialize SDL_mixer
	StdSdlSubSystem system{SDL_INIT_AUDIO};
	ThrowIfFailed("Mix_OpenAudio",
		Mix_OpenAudio(Frequency, Format, NumChannels, 1024) != 0);
	Mix_AllocateChannels(Config.Sound.MaxChannels);
	this->system.emplace(std::move(system));
}

void C4AudioSystem::ThrowIfFailed(const char *const funcName, const bool failed)
{
	if (failed)
	{
		throw std::runtime_error(std::string{"SDL_mixer: "} +
			funcName + " failed: " + Mix_GetError());
	}
}

void C4AudioSystem::FadeOutMusic(const std::int32_t ms)
{
	ThrowIfFailed("Mix_FadeOutMusic", Mix_FadeOutMusic(ms) != 1);
}

bool C4AudioSystem::IsMusicPlaying() const
{
	return Mix_PlayingMusic() == 1;
}

void C4AudioSystem::PlayMusic(const MusicFile &music, const bool loop)
{
	ThrowIfFailed("Mix_PlayMusic", Mix_PlayMusic(music.sample.get(), (loop ? -1 : 1)) == -1);
}

void C4AudioSystem::SetMusicVolume(const float volume)
{
	Mix_VolumeMusic(std::lrint(volume * MIX_MAX_VOLUME));
}

void C4AudioSystem::StopMusic()
{
	Mix_HaltMusic();
}

void C4AudioSystem::UnpauseMusic() { /* Not supported */ }

C4AudioSystem::MusicFile::MusicFile(const void *const buf, const std::size_t size)
	: sample{Mix_LoadMUS_RW(SDL_RWFromConstMem(buf, size), SDL_TRUE), Mix_FreeMusic}
{
	ThrowIfFailed("Mix_LoadMUS_RW", !sample);
}

C4AudioSystem::SoundFile::SoundFile(const void *const buf, const std::size_t size)
	: sample{Mix_LoadWAV_RW(SDL_RWFromConstMem(buf, size), SDL_TRUE), Mix_FreeChunk}
{
	ThrowIfFailed("Mix_LoadWAV_RW", !sample);
}

std::uint32_t C4AudioSystem::SoundFile::GetDuration() const
{
	return 1000 * sample->alen / BytesPerSecond;
}

C4AudioSystem::SoundChannel::SoundChannel(const C4AudioSystem::SoundFile &sound, const bool loop)
	: channel{Mix_PlayChannel(-1, sound.sample.get(), (loop ? -1 : 0))}
{
	ThrowIfFailed("Mix_PlayChannel", channel == -1);
}

C4AudioSystem::SoundChannel::~SoundChannel()
{
	Mix_HaltChannel(channel);
}

bool C4AudioSystem::SoundChannel::IsPlaying() const
{
	return Mix_Playing(channel) == 1;
}

void C4AudioSystem::SoundChannel::SetPosition(const std::uint32_t ms)
{
	// Not supported
}

void C4AudioSystem::SoundChannel::SetVolumeAndPan(const float volume, const float pan)
{
	Mix_Volume(channel, std::lrint(volume * MIX_MAX_VOLUME));
	const Uint8
		left  = std::clamp(std::lrint((1.0f - pan) * 255.0f), 0L, 255L),
		right = std::clamp(std::lrint((1.0f + pan) * 255.0f), 0L, 255L);
	ThrowIfFailed("Mix_SetPanning", Mix_SetPanning(channel, left, right) == 0);
}

void C4AudioSystem::SoundChannel::Unpause() { /* Not supported */ }
