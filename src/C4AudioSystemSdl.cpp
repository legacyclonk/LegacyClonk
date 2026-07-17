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
	MIX_Track *GetFreeAudioTrack();
	void ReturnAudioTrack(MIX_Track *track);
	SDL_PropertiesID loopProperty;
	SDL_PropertiesID noLoopProperty;
	std::shared_ptr<spdlog::logger> logger;

private:
	static constexpr int frequency = 44100;
	static constexpr SDL_AudioFormat format = SDL_AUDIO_S16;
	static constexpr int numChannels = 2;
	static constexpr int bytesPerSecond =
	frequency * (SDL_AUDIO_BITSIZE(format) / 8) * numChannels;

	std::optional<StdSdlSubSystem> sdlSubsystem;

	static void ThrowIfFailed(const char *funcName, bool failed, std::string_view errorMessage = {});

	static MIX_Audio *LoadSampleCheckMpegLayer3Header(const void *buf, const std::size_t size);

	MIX_Mixer *mixer;
	MIX_Track *musicTrack;
	std::vector<MIX_Track*> audioTracks;

public:

	class MusicFileSdl : public MusicFile
	{
	public:
		MusicFileSdl(const void *buf, std::size_t size);

	private:
		MIX_Audio *sample;

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
		MIX_Track *GetAssignedTrack()
		{
			return assignedTrack;
		};

	private:
		MIX_Track *assignedTrack;
		MIX_StereoGains stereoGains;
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
		MIX_Audio *sample;

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
static constexpr auto maximumMusicVolume = 80;

// higher than MaximumMusicVolume to compensate for lower maximum panning volume
static constexpr auto maximumSoundVolume = 100;

const char* GetAudioFormatString(const SDL_AudioFormat& audioFormat)
{
	switch (audioFormat)
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
	std::int32_t compile_version{MIX_Version()};
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
	const SDL_AudioSpec audioSpec{format, numChannels, frequency};
	mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audioSpec);
	ThrowIfFailed("MIX_CreateMixerDevice", mixer == nullptr);

	SDL_AudioSpec spec;
	MIX_GetMixerFormat(mixer, &spec);
	logger->debug("SDL_mixer device spec: frequency = {} Hz, format = {}, channels = {}", spec.freq, GetAudioFormatString(spec.format), spec.channels);

	this->sdlSubsystem.emplace(std::move(system));
	audioTracks.reserve(maxTracks);
	for (std::int32_t trackIndex{0}; trackIndex < maxTracks; ++trackIndex)
	{
		audioTracks.emplace_back(MIX_CreateTrack(mixer));
		MIX_SetTrackStoppedCallback(audioTracks.back(), TrackFinished, nullptr);
	}
	musicTrack = MIX_CreateTrack(mixer);

	loopProperty = SDL_CreateProperties();
	SDL_SetNumberProperty(loopProperty, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
	noLoopProperty = SDL_CreateProperties();
	SDL_SetNumberProperty(noLoopProperty, MIX_PROP_PLAY_LOOPS_NUMBER, 0);
}

C4AudioSystemSdl::~C4AudioSystemSdl() noexcept
{
	MIX_DestroyMixer(mixer); // Will also destroy audio tracks.
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
	ThrowIfFailed("MIX_StopTrack", MIX_StopTrack(musicTrack, MIX_TrackMSToFrames(musicTrack, ms)) != 1);
}

bool C4AudioSystemSdl::IsMusicPlaying() const
{
	return MIX_TrackPlaying(musicTrack);
}

void C4AudioSystemSdl::PlayMusic(const C4AudioSystem::MusicFile *const music, const bool loop)
{
	MIX_SetTrackAudio(musicTrack, static_cast<const MusicFileSdl *>(music)->sample);
	ThrowIfFailed("MIX_PlayTrack", !MIX_PlayTrack(musicTrack, loop ? loopProperty : noLoopProperty));
}

void C4AudioSystemSdl::SetMusicVolume(const float volume)
{
	MIX_SetTrackGain(musicTrack, volume * (maximumMusicVolume / 100.0f));
}

void C4AudioSystemSdl::StopMusic()
{
	MIX_StopTrack(musicTrack, MIX_TrackMSToFrames(musicTrack, 100));
}

void C4AudioSystemSdl::UnpauseMusic()
{
	MIX_ResumeTrack(musicTrack);
}

MIX_Track* C4AudioSystemSdl::GetFreeAudioTrack()
{
	if (audioTracks.size())
	{
		MIX_Track *freeTrack{audioTracks.back()};
		audioTracks.pop_back();
		return freeTrack;
	}
	return nullptr;
}

void C4AudioSystemSdl::ReturnAudioTrack(MIX_Track *track)
{
	audioTracks.push_back(track);
}

MIX_Audio *C4AudioSystemSdl::LoadSampleCheckMpegLayer3Header(const void *const buf, const std::size_t size)
{
	if(!C4AudioSystemSdl::instance)
	{
		ThrowIfFailed("LoadSampleCheckMpegLayer3Header", true, "C4AudioSystemSdl Instance invalid.");
		return nullptr;
	}

	MIX_Audio *const direct{MIX_LoadAudio_IO(C4AudioSystemSdl::instance->mixer, SDL_IOFromConstMem(buf, size), true, true)};
	if (direct)
	{
		return direct;
	}
	const std::string error{SDL_GetError()};

	// According to http://www.idea2ic.com/File_Formats/MPEG%20Audio%20Frame%20Header.pdf
	// Maximum possible frame size = 144 * max bit rate / min sample rate + padding
	// chosen values are limited to layer 3
	static constexpr std::size_t maxFrameSize{144 * 320'000 / 8'000 + 1};

	const std::span data{reinterpret_cast<const std::byte *>(buf), size};
	const std::size_t limit{std::min(data.size(), maxFrameSize)};

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
		MIX_Audio *const sample{MIX_LoadAudio_IO(C4AudioSystemSdl::instance->mixer, SDL_IOFromConstMem(data.data() + i, size - i), true, true)};
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

	stereoGains.left = 0.0f;
	stereoGains.right = 0.0f;

	MIX_Track *track{C4AudioSystemSdl::instance->GetFreeAudioTrack()};
	if(track)
	{
		this->assignedTrack = track;
		ThrowIfFailed("MIX_SetTrackAudio", !MIX_SetTrackAudio(track, sound->sample));
		ThrowIfFailed("MIX_PlayTrack", !MIX_PlayTrack(track, loop ? C4AudioSystemSdl::instance->loopProperty : C4AudioSystemSdl::instance->noLoopProperty));
		MIX_PauseTrack(track); // Unpaused in outer AudioSystem
	}
}

C4AudioSystemSdl::SoundChannelSdl::~SoundChannelSdl()
{
	if(C4AudioSystemSdl::instance && assignedTrack)
	{
		MIX_StopTrack(assignedTrack, 0);
		C4AudioSystemSdl::instance->ReturnAudioTrack(assignedTrack);
	}
}

bool C4AudioSystemSdl::SoundChannelSdl::IsPlaying() const
{
	if(assignedTrack)
	{
		return MIX_TrackPlaying(assignedTrack);
	}
	return false;
}

void C4AudioSystemSdl::SoundChannelSdl::SetPosition(const std::uint32_t ms)
{
	if(assignedTrack)
	{
		MIX_SetTrackPlaybackPosition(assignedTrack, MIX_TrackMSToFrames(assignedTrack, ms));
	}
}

void C4AudioSystemSdl::SoundChannelSdl::SetVolumeAndPan(const float volume, const float pan)
{
	if(!assignedTrack)
	{
		return;
	}
	ThrowIfFailed("MIX_SetTrackGain", !MIX_SetTrackGain(assignedTrack, volume * (maximumSoundVolume / 100.0f)));
	stereoGains.left = 1.0f - pan;
	stereoGains.right = 1.0f + pan;
	ThrowIfFailed("MIX_SetTrackStereo", !MIX_SetTrackStereo(assignedTrack, &stereoGains));
}

void C4AudioSystemSdl::SoundChannelSdl::Unpause()
{
	if(assignedTrack)
	{
		MIX_ResumeTrack(assignedTrack);
	}
}

void C4AudioSystemSdl::TrackFinished(void *userdata, MIX_Track *track)
{
}

C4AudioSystem *CreateC4AudioSystemSdl(int maxChannels, const bool preferLinearResampling)
{
	return new C4AudioSystemSdl{maxChannels, preferLinearResampling};
}
