/*
 * LegacyClonk
 *
 * Copyright (c) 2017-2024, The LegacyClonk Team and contributors
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

#include "C4Application.h"
#include "C4AudioSystemSdl.h"
#include "C4Config.h"
#include "C4Log.h"

#include "StdHelpers.h"
#include "StdSdlSubSystem.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <stdexcept>
#include <optional>
#include <span>

#include <SDL3_mixer/SDL_mixer.h>

class C4AudioSystemSdl : public C4AudioSystem
{
public:
	C4AudioSystemSdl(int maxTracks, bool preferLinearResampling);
	~C4AudioSystemSdl() noexcept override;

	void FadeOutMusic(std::int32_t ms) override;
	bool IsMusicPlaying() const override;
	void PlayMusic(const MusicFile *const music, bool loop) override;
	void SetMusicVolume(float volume) override;
	void StopMusic() override;
	void UnpauseMusic() override;
	MIX_Track* GetFreeAudioTrack();
	void ReturnAudioTrack(MIX_Track* Track);
	SDL_PropertiesID LoopProperty;
	SDL_PropertiesID NoLoopProperty;
	std::shared_ptr<spdlog::logger> logger;

private:
	static constexpr int Frequency = 44100;
	static constexpr SDL_AudioFormat Format = SDL_AUDIO_S16;
	static constexpr int NumChannels = 2;
	static constexpr int BytesPerSecond =
	Frequency * (SDL_AUDIO_BITSIZE(Format) / 8) * NumChannels;

	std::optional<StdSdlSubSystem> system;

	static void ThrowIfFailed(const char *funcName, bool failed, std::string_view errorMessage = {});

	static MIX_Audio *LoadSampleCheckMpegLayer3Header(const void *buf, const std::size_t size);

	MIX_Mixer* Mixer;
	MIX_Track* MusicTrack;
	std::vector<MIX_Track*> AudioTracks;

public:

	class MusicFileSdl : public MusicFile
	{
	public:
		MusicFileSdl(const void *buf, std::size_t size);

	private:
		MIX_Audio* sample;

		friend class C4AudioSystemSdl;
	};

	MusicFile *CreateMusicFile(const void *buf, std::size_t size) override
	{
		return new MusicFileSdl{buf, size};
	}

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
		MIX_Track* GetAssignedTrack()
		{
			return AssignedTrack;
		};

	private:
		MIX_Track* AssignedTrack;
		MIX_StereoGains StereoGains;
	};

	SoundChannel *CreateSoundChannel(const SoundFile *const sound, bool loop) override
	{
		// Caller manages lifetime.
		const auto channel = new SoundChannelSdl{static_cast<const SoundFileSdl *>(sound), loop};
		return channel;
	}

	class SoundFileSdl : public SoundFile
	{
	public:
		SoundFileSdl(const void *buf, std::size_t size);

	public:
		std::uint32_t GetDuration() const override;

	private:
		MIX_Audio* sample;

		friend class C4AudioSystemSdl;
	};

	virtual SoundFile *CreateSoundFile(const void *buf, std::size_t size) override { return new SoundFileSdl{buf, size}; }

private:
	static inline C4AudioSystemSdl *instance{nullptr};
	static void TrackFinished(void *userdata, MIX_Track *track);
};

// this is used instead of MIX_MAX_VOLUME, because MIX_MAX_VOLUME is very loud and easily leads to clipping
// the lower volume gives more headroom until clipping occurs
// the selected volume is chosen to be similar to FMod's original volume
static constexpr auto MaximumMusicVolume = 80;

// higher than MaximumMusicVolume to compensate for lower maximum panning volume
static constexpr auto MaximumSoundVolume = 100;

const char* GetAudioFormatString(const SDL_AudioFormat& AudioFormat)
{
	switch (AudioFormat)
	{
	case SDL_AUDIO_UNKNOWN:
		return "SDL_AUDIO_UNKNOWN";
	case SDL_AUDIO_U8:
		return "SDL_AUDIO_U8";
	case SDL_AUDIO_S8:
		return "SDL_AUDIO_S8";
	case SDL_AUDIO_S16LE:
		return "SDL_AUDIO_S16LE";
	case SDL_AUDIO_S16BE:
		return "SDL_AUDIO_S16BE";
	case SDL_AUDIO_S32LE:
		return "SDL_AUDIO_S32LE";
	case SDL_AUDIO_S32BE:
		return "SDL_AUDIO_S32BE";
	case SDL_AUDIO_F32LE:
		return "SDL_AUDIO_F32LE";
	case SDL_AUDIO_F32BE:
		return "SDL_AUDIO_F32BE";
	}
	return "None";
}

C4AudioSystemSdl::C4AudioSystemSdl(const int maxTracks, const bool preferLinearResampling)
{
	assert(!instance);
	instance = this;

	logger = Application.LogSystem.CreateLoggerWithDifferentName(Config.Logging.AudioSystem, "C4AudioSystem");

	// Check SDL_mixer version
	std::int32_t compile_version = MIX_Version();
	logger->info("SDL_mixer runtime version is {}.{}.{} (compiled with {})",
		SDL_MIXER_MAJOR_VERSION, SDL_MIXER_MINOR_VERSION, SDL_MIXER_MICRO_VERSION,
		compile_version);


	// TODO: This hint doesn't exist anymore in SDL3. Also linear seems to never have been a valid option.
	/*
	// Try to enable linear resampling if requested
	if (preferLinearResampling)
	{
		if (!SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "linear"))
		{
			logger->error("SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, \"linear\") failed");
		}
	}
	*/

	// Initialize SDL_mixer
	StdSdlSubSystem system{SDL_INIT_AUDIO};
	ThrowIfFailed("MIX_Init", !MIX_Init());
	const SDL_AudioSpec AudioSpec{Format, NumChannels, Frequency};
	Mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &AudioSpec);
	ThrowIfFailed("MIX_CreateMixerDevice", Mixer == nullptr);

	SDL_AudioSpec Spec;
	MIX_GetMixerFormat(Mixer, &Spec);
	logger->debug("SDL_mixer device spec: frequency = {} Hz, format = {}, channels = {}", Spec.freq, GetAudioFormatString(Spec.format), Spec.channels);

	this->system.emplace(std::move(system));
	AudioTracks.reserve(maxTracks);
	for (int TrackIndex = 0; TrackIndex < maxTracks; ++TrackIndex)
	{
		AudioTracks.emplace_back(MIX_CreateTrack(Mixer));
		MIX_SetTrackStoppedCallback(AudioTracks.back(), TrackFinished, nullptr);
	}
	MusicTrack = MIX_CreateTrack(Mixer);

	LoopProperty = SDL_CreateProperties();
	SDL_SetNumberProperty(LoopProperty, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
	NoLoopProperty = SDL_CreateProperties();
	SDL_SetNumberProperty(NoLoopProperty, MIX_PROP_PLAY_LOOPS_NUMBER, 0);
}

C4AudioSystemSdl::~C4AudioSystemSdl() noexcept
{
	MIX_DestroyMixer(Mixer); // Will also destroy audio tracks.
	MIX_Quit();
}

void C4AudioSystemSdl::ThrowIfFailed(const char *const funcName, const bool failed, std::string_view errorMessage)
{
	if (failed)
	{
		if (errorMessage.empty())
		{
			errorMessage = SDL_GetError();
		}

		throw std::runtime_error{std::format("SDL_mixer: {} failed: {}", funcName, errorMessage)};
	}
}

void C4AudioSystemSdl::FadeOutMusic(const std::int32_t ms)
{
	ThrowIfFailed("MIX_StopTrack", MIX_StopTrack(MusicTrack, MIX_TrackMSToFrames(MusicTrack, ms)) != 1);
}

bool C4AudioSystemSdl::IsMusicPlaying() const
{
	return MIX_TrackPlaying(MusicTrack);
}

void C4AudioSystemSdl::PlayMusic(const C4AudioSystem::MusicFile *const music, const bool loop)
{
	MIX_SetTrackAudio(MusicTrack, static_cast<const MusicFileSdl *>(music)->sample);
	ThrowIfFailed("MIX_PlayTrack", !MIX_PlayTrack(MusicTrack, loop ? LoopProperty : NoLoopProperty));
}

void C4AudioSystemSdl::SetMusicVolume(const float volume)
{
	MIX_SetTrackGain(MusicTrack, volume * (MaximumMusicVolume / 100.0f));
}

void C4AudioSystemSdl::StopMusic()
{
	MIX_StopTrack(MusicTrack, MIX_TrackMSToFrames(MusicTrack, 100));
}

void C4AudioSystemSdl::UnpauseMusic()
{
	MIX_ResumeTrack(MusicTrack);
}

MIX_Track* C4AudioSystemSdl::GetFreeAudioTrack()
{
	if (AudioTracks.size())
	{
		MIX_Track* FreeTrack = AudioTracks.back();
		AudioTracks.pop_back();
		return FreeTrack;
	}
	return nullptr;
}

void C4AudioSystemSdl::ReturnAudioTrack(MIX_Track* Track)
{
	AudioTracks.push_back(Track);
}

MIX_Audio *C4AudioSystemSdl::LoadSampleCheckMpegLayer3Header(const void *const buf, const std::size_t size)
{
	if(!C4AudioSystemSdl::instance)
	{
		ThrowIfFailed("LoadSampleCheckMpegLayer3Header", true, "C4AudioSystemSdl Instance invalid.");
		return nullptr;
	}

	MIX_Audio* const direct = MIX_LoadAudio_IO(C4AudioSystemSdl::instance->Mixer, SDL_IOFromConstMem(buf, size), true, true);
	if (direct)
	{
		return direct;
	}
	const std::string error{SDL_GetError()};

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
		MIX_Audio* const sample = MIX_LoadAudio_IO(C4AudioSystemSdl::instance->Mixer, SDL_IOFromConstMem(data.data() + i, size - i), true, true);
		if (sample)
		{
			return sample;
		}
	}

	ThrowIfFailed("MIX_LoadAudio", true, error);
	return nullptr;
}

C4AudioSystemSdl::MusicFileSdl::MusicFileSdl(const void *const buf, const std::size_t size)
	: sample{LoadSampleCheckMpegLayer3Header(buf, size)}
{}

C4AudioSystemSdl::SoundFileSdl::SoundFileSdl(const void *const buf, const std::size_t size)
	: sample{LoadSampleCheckMpegLayer3Header(buf, size)}
{}

std::uint32_t C4AudioSystemSdl::SoundFileSdl::GetDuration() const
{
	return MIX_AudioFramesToMS(sample, MIX_GetAudioDuration(sample));
}

C4AudioSystemSdl::SoundChannelSdl::SoundChannelSdl(const SoundFileSdl *const sound, const bool loop)
{
	if(!C4AudioSystemSdl::instance)
	{
		ThrowIfFailed("SoundChannelSdl", true, "C4AudioSystemSdl Instance invalid.");
		return;
	}

	StereoGains.left = 0.0f;
	StereoGains.right = 0.0f;

	MIX_Track* Track = C4AudioSystemSdl::instance->GetFreeAudioTrack();
	if(Track)
	{
		this->AssignedTrack = Track;
		ThrowIfFailed("MIX_SetTrackAudio", !MIX_SetTrackAudio(Track, sound->sample));
		ThrowIfFailed("MIX_PlayTrack", !MIX_PlayTrack(Track, loop ? C4AudioSystemSdl::instance->LoopProperty : C4AudioSystemSdl::instance->NoLoopProperty));
		MIX_PauseTrack(Track); // Unpaused in outer AudioSystem
	}
}

C4AudioSystemSdl::SoundChannelSdl::~SoundChannelSdl()
{
	if(C4AudioSystemSdl::instance && AssignedTrack)
	{
		MIX_StopTrack(AssignedTrack, 0);
		C4AudioSystemSdl::instance->ReturnAudioTrack(AssignedTrack);
	}
}

bool C4AudioSystemSdl::SoundChannelSdl::IsPlaying() const
{
	if(AssignedTrack)
	{
		return MIX_TrackPlaying(AssignedTrack);
	}
	return false;
}

void C4AudioSystemSdl::SoundChannelSdl::SetPosition(const std::uint32_t ms)
{
	if(AssignedTrack)
	{
		MIX_SetTrackPlaybackPosition(AssignedTrack, MIX_TrackMSToFrames(AssignedTrack, ms));
	}
}

void C4AudioSystemSdl::SoundChannelSdl::SetVolumeAndPan(const float volume, const float pan)
{
	if(!AssignedTrack)
	{
		return;
	}
	ThrowIfFailed("MIX_SetTrackGain", !MIX_SetTrackGain(AssignedTrack, volume * (MaximumSoundVolume / 100.0f)));
	StereoGains.left = 1.0f - pan;
	StereoGains.right = 1.0f + pan;
	ThrowIfFailed("MIX_SetTrackStereo", !MIX_SetTrackStereo(AssignedTrack, &StereoGains));
}

void C4AudioSystemSdl::SoundChannelSdl::Unpause()
{
	if(AssignedTrack)
	{
		MIX_ResumeTrack(AssignedTrack);
	}
}

void C4AudioSystemSdl::TrackFinished(void *userdata, MIX_Track *track)
{
}

C4AudioSystem *CreateC4AudioSystemSdl(int maxChannels, const bool preferLinearResampling)
{
	return new C4AudioSystemSdl{maxChannels, preferLinearResampling};
}
