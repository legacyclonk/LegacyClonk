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

#include "C4AudioSystemSdl.h"

#include "C4Log.h"

#include "Standard.h"
#include "StdHelpers.h"
#include "StdSdlSubSystem.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>
#include <memory>
#include <optional>
#include <span>

#include <SDL_mixer.h>

class C4AudioSystemSdl : public C4AudioSystem
{
public:
	C4AudioSystemSdl(int maxChannels, bool preferLinearResampling);

	void FadeOutMusic(std::int32_t ms) override;
	bool IsMusicPlaying() const override;
	void PlayMusic(const MusicFile *const music, bool loop) override;
	void SetMusicVolume(float volume) override;
	void StopMusic() override;
	void UnpauseMusic() override;

private:
	static constexpr int Frequency = 44100;
	static constexpr Uint16 Format = AUDIO_S16SYS;
	static constexpr int NumChannels = 2;
	static constexpr int BytesPerSecond =
	Frequency * (16 / 8) * NumChannels;

	// Smart pointers for SDL_mixer objects
	using SDLMixChunkUniquePtr = C4DeleterFunctionUniquePtr<Mix_FreeChunk>;
	using SDLMixMusicUniquePtr = C4DeleterFunctionUniquePtr<Mix_FreeMusic>;

	std::optional<StdSdlSubSystem> system;

	static void ThrowIfFailed(const char *funcName, bool failed, std::string_view errorMessage = {});

	template <typename T, typename... extra>
	using SampleLoadFunc = T *(*)(SDL_RWops *, extra...);

	template <typename T, typename... extra>
	static T *LoadSampleCheckMpegLayer3Header(SampleLoadFunc<T, extra...> loadFunc, const char *funcName, const void *buf, const std::size_t size);

public:

	class MusicFileSdl : public MusicFile
	{
	public:
		MusicFileSdl(const void *buf, std::size_t size);

	private:
		SDLMixMusicUniquePtr sample;

		friend class C4AudioSystemSdl;
	};

	MusicFile *CreateMusicFile(const void *buf, std::size_t size) override { return new MusicFileSdl{buf, size}; }

	class SoundFileSdl;

	class SoundChannelSdl : public SoundChannel
	{
	public:
		// Plays sound file. If supported by audio library, playback starts paused.
		SoundChannelSdl(const SoundFileSdl *const sound, bool loop);
		~SoundChannelSdl() override;

	public:
		bool IsPlaying() const override;
		void SetPosition(std::uint32_t ms) override;
		void SetVolumeAndPan(float volume, float pan) override;
		void Unpause() override;

	private:
		const int channel;
	};

	SoundChannel *CreateSoundChannel(const SoundFile *const sound, bool loop) override { return new SoundChannelSdl{static_cast<const SoundFileSdl *>(sound), loop}; }

	class SoundFileSdl : public SoundFile
	{
	public:
		SoundFileSdl(const void *buf, std::size_t size);

	public:
		std::uint32_t GetDuration() const override;

	private:
		const SDLMixChunkUniquePtr sample;

		friend class C4AudioSystemSdl;
	};

	virtual SoundFile *CreateSoundFile(const void *buf, std::size_t size) override { return new SoundFileSdl{buf, size}; }
};

// this is used instead of MIX_MAX_VOLUME, because MIX_MAX_VOLUME is very loud and easily leads to clipping
// the lower volume gives more headroom until clipping occurs
// the selected volume is chosen to be similar to FMod's original volume
static constexpr auto MaximumVolume = 80;

C4AudioSystemSdl::C4AudioSystemSdl(const int maxChannels, const bool preferLinearResampling)
{
	// Check SDL_mixer version
	SDL_version compile_version;
	MIX_VERSION(&compile_version);
	const auto link_version = Mix_Linked_Version();
	LogF("SDL_mixer runtime version is %d.%d.%d (compiled with %d.%d.%d)",
		link_version->major, link_version->minor, link_version->patch,
		compile_version.major, compile_version.minor, compile_version.patch);

	// Try to enable linear resampling if requested
	if (preferLinearResampling)
	{
		//if (!SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "linear"))
//			LogF("SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, \"linear\") failed");
	}

	// Initialize SDL_mixer
	StdSdlSubSystem system{SDL_INIT_AUDIO};
	ThrowIfFailed("Mix_OpenAudio",
		Mix_OpenAudio(Frequency, Format, NumChannels, 1024) != 0);
	Mix_AllocateChannels(maxChannels);
	this->system.emplace(std::move(system));
}

void C4AudioSystemSdl::ThrowIfFailed(const char *const funcName, const bool failed, std::string_view errorMessage)
{
	if (failed)
	{
		if (errorMessage.empty())
		{
			errorMessage = Mix_GetError();
		}

		throw std::runtime_error{std::format("SDL_mixer: {} failed: {}", funcName, errorMessage)};
	}
}

void C4AudioSystemSdl::FadeOutMusic(const std::int32_t ms)
{
	ThrowIfFailed("Mix_FadeOutMusic", Mix_FadeOutMusic(ms) != 1);
}

bool C4AudioSystemSdl::IsMusicPlaying() const
{
	return Mix_PlayingMusic() == 1;
}

void C4AudioSystemSdl::PlayMusic(const C4AudioSystem::MusicFile *const music, const bool loop)
{
	ThrowIfFailed("Mix_PlayMusic", Mix_PlayMusic(static_cast<const MusicFileSdl *const>(music)->sample.get(), (loop ? -1 : 1)) == -1);
}

void C4AudioSystemSdl::SetMusicVolume(const float volume)
{
	Mix_VolumeMusic(std::lrint(volume * MaximumVolume));
}

void C4AudioSystemSdl::StopMusic()
{
	Mix_HaltMusic();
}

void C4AudioSystemSdl::UnpauseMusic() { /* Not supported */ }

template <typename T, typename... extra>
T *C4AudioSystemSdl::LoadSampleCheckMpegLayer3Header(const SampleLoadFunc<T, extra...> loadFunc, const char *const funcName, const void *const buf, const std::size_t size)
{
	const auto loadIt = [loadFunc](auto buf, auto size)
	{
		if constexpr (sizeof...(extra) == 1)
		{
			return loadFunc(SDL_RWFromConstMem(buf, size), SDL_TRUE);
		}
		else
		{
			return loadFunc(SDL_RWFromConstMem(buf, size));
		}
	};
	const auto direct = loadIt(buf, size);
	if (direct)
	{
		return direct;
	}
	const std::string error{Mix_GetError()};

	// According to http://www.idea2ic.com/File_Formats/MPEG%20Audio%20Frame%20Header.pdf
	// Maximum possible frame size = 144 * max bit rate / min sample rate + padding
	// chosen values are limited to layer 3
	static constexpr std::size_t MaxFrameSize{144 * 320'000 / 8'000 + 1};

	const std::span data{reinterpret_cast<const std::byte *>(buf), size};
	const std::size_t limit{std::min(data.size(), MaxFrameSize)};

	for (std::size_t i{0}; i < limit - 4; ++i)
	{
		// first 8 of 11 frame sync bits
		if (data[i] != std::byte{0xFF}) continue;

		const auto byte2 = data[i + 1];
		// rest of the sync bits + check for Layer 3 (SDL_mixer only accepts layer 3)
		if ((byte2 & std::byte{0xE6}) != std::byte{0xE2}) continue;
		// MPEG version bit value 01 is reserved
		if ((byte2 & std::byte{0x18}) == std::byte{0x08}) continue;

		const auto byte3 = data[i + 2];
		// bitrate index 1111 is invalid
		if ((byte3 & std::byte{0xF0}) == std::byte{0xF0}) continue;
		// sampling rate index 11 is reserved
		if ((byte3 & std::byte{0x0C}) == std::byte{0x0C}) continue;

		const auto byte4 = data[i + 3];
		// emphasis bit value 10 is reserved
		if ((byte4 & std::byte{0x03}) == std::byte{0x02}) continue;

		// at this point there seems to be a valid MPEG frame header
		const auto sample = loadIt(buf, size);
		if (sample)
		{
			return sample;
		}
	}

	ThrowIfFailed(funcName, true, error);
	return nullptr;
}

C4AudioSystemSdl::MusicFileSdl::MusicFileSdl(const void *const buf, const std::size_t size)
	: sample{LoadSampleCheckMpegLayer3Header(Mix_LoadMUS_RW, "Mix_LoadMUS_RW", buf, size)}
{}

C4AudioSystemSdl::SoundFileSdl::SoundFileSdl(const void *const buf, const std::size_t size)
	: sample{LoadSampleCheckMpegLayer3Header(Mix_LoadWAV_RW, "Mix_LoadWAV_RW", buf, size)}
{}

std::uint32_t C4AudioSystemSdl::SoundFileSdl::GetDuration() const
{
	return 1000 * sample->alen / BytesPerSecond;
}

C4AudioSystemSdl::SoundChannelSdl::SoundChannelSdl(const SoundFileSdl *const sound, const bool loop)
	: channel{Mix_PlayChannel(-1, sound->sample.get(), (loop ? -1 : 0))}
{
	ThrowIfFailed("Mix_PlayChannel", channel == -1);
}

C4AudioSystemSdl::SoundChannelSdl::~SoundChannelSdl()
{
	Mix_HaltChannel(channel);
}

bool C4AudioSystemSdl::SoundChannelSdl::IsPlaying() const
{
	return Mix_Playing(channel) == 1;
}

void C4AudioSystemSdl::SoundChannelSdl::SetPosition(const std::uint32_t ms)
{
	// Not supported
}

void C4AudioSystemSdl::SoundChannelSdl::SetVolumeAndPan(const float volume, const float pan)
{
	Mix_Volume(channel, std::lrint(volume * MaximumVolume));
	const Uint8
		left  = static_cast<Uint8>(std::clamp(std::lrint((1.0f - pan) * 255.0f), 0L, 255L)),
		right = static_cast<Uint8>(std::clamp(std::lrint((1.0f + pan) * 255.0f), 0L, 255L));
	ThrowIfFailed("Mix_SetPanning", Mix_SetPanning(channel, left, right) == 0);
}

void C4AudioSystemSdl::SoundChannelSdl::Unpause() { /* Not supported */ }

C4AudioSystem *CreateC4AudioSystemSdl(int maxChannels, const bool preferLinearResampling)
{
	return new C4AudioSystemSdl{maxChannels, preferLinearResampling};
}
