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

// Handles music and sound playback via FMOD 1.10 or SDL2_mixer.

#pragma once

#include <Standard.h>

#include <cstddef>
#include <cstdint>

#ifdef USE_FMOD
#include <fmod.hpp>
#include <fmod_errors.h>
#include <memory>
#elif defined(USE_SDL_MIXER)
#include <StdSdlSubSystem.h>
#include <SDL_mixer.h>
#include <memory>
#include <optional>
#endif

class C4AudioSystem
{
public:
	C4AudioSystem();
	C4AudioSystem(const C4AudioSystem &) = delete;
	C4AudioSystem(C4AudioSystem &&) = delete;
	~C4AudioSystem() = default;
	C4AudioSystem &operator=(const C4AudioSystem &) = delete;

#ifdef USE_FMOD

private:
	// Deleters for FMOD objects
	template<typename T>
	static void ReleaseFmodObj(T *const obj) { obj->release(); }
	static void StopFmodChannel(FMOD::Channel *const channel) { channel->stop(); }

	// Smart pointers for FMOD objects
	template<typename T>
	using FmodUniquePtr = std::unique_ptr<T, decltype(&ReleaseFmodObj<T>)>;
	using FmodChannelUniquePtr = std::unique_ptr<FMOD::Channel, decltype(&StopFmodChannel)>;

	FmodUniquePtr<FMOD::System> system{nullptr, ReleaseFmodObj<FMOD::System>};
	FmodChannelUniquePtr musicChannel{nullptr, StopFmodChannel};

	FmodUniquePtr<FMOD::Sound> FmodCreateSound(const void *buf, std::size_t size, bool decompress);
	FmodChannelUniquePtr FmodPlaySound(FMOD::Sound &sound, bool loop, bool maxPriority);

	static void ThrowIfFailed(const char *funcName, FMOD_RESULT result);

#elif defined(USE_SDL_MIXER)

private:
	static constexpr int Frequency = 44100;
	static constexpr Uint16 Format = AUDIO_S16SYS;
	static constexpr int NumChannels = 2;
	static constexpr int BytesPerSecond =
		Frequency * (SDL_AUDIO_BITSIZE(Format) / 8) * NumChannels;

	// Smart pointers for SDL_mixer objects
	using SDLMixChunkUniquePtr = std::unique_ptr<Mix_Chunk, decltype(&Mix_FreeChunk)>;
	using SDLMixMusicUniquePtr = std::unique_ptr<Mix_Music, decltype(&Mix_FreeMusic)>;

	std::optional<StdSdlSubSystem> system;

	static void ThrowIfFailed(const char *funcName, bool failed);

#endif

public:
	class MusicFile;
	class SoundFile;

	void FadeOutMusic(std::int32_t ms);
	bool IsMusicPlaying() const;
	// Plays music file. If supported by audio library, playback starts paused.
	void PlayMusic(const MusicFile &music, bool loop);
	// Range: 0.0f through 1.0f
	void SetMusicVolume(float volume);
	void StopMusic();
	void UnpauseMusic();

	class MusicFile
	{
	public:
		MusicFile() = delete;
		MusicFile(const void *buf, std::size_t size);
		MusicFile(const MusicFile &) = delete;
		MusicFile(MusicFile &&) = default;
		~MusicFile() = default;
		MusicFile &operator=(const MusicFile &) = delete;

	private:
#ifdef USE_FMOD
		const FmodUniquePtr<FMOD::Sound> sample;
#elif defined(USE_SDL_MIXER)
		const SDLMixMusicUniquePtr sample;
#endif

		friend class C4AudioSystem;
	};

	class SoundChannel
	{
	public:
		SoundChannel() = delete;
		// Plays sound file. If supported by audio library, playback starts paused.
		SoundChannel(const SoundFile &sound, bool loop);
		SoundChannel(const SoundChannel &) = delete;
		SoundChannel(SoundChannel &&) = delete;
#ifdef USE_SDL_MIXER
		~SoundChannel();
#else
		~SoundChannel() = default;
#endif
		SoundChannel &operator=(const SoundChannel &) = delete;

		bool IsPlaying() const;
		void SetPosition(std::uint32_t ms);
		// volume range: 0.0f through 1.0f; pan range: -1.0f through 1.0f
		void SetVolumeAndPan(float volume, float pan);
		void Unpause();

	private:
#ifdef USE_FMOD
		const FmodChannelUniquePtr channel;
#elif defined(USE_SDL_MIXER)
		const int channel;
#endif
	};

	class SoundFile
	{
	public:
		SoundFile() = delete;
		SoundFile(const void *buf, std::size_t size);
		SoundFile(const SoundFile &) = delete;
		SoundFile(SoundFile &&) = delete;
		~SoundFile() = default;
		SoundFile &operator=(const SoundFile &) = delete;

		std::uint32_t GetDuration() const;

	private:
#ifdef USE_FMOD
		const FmodUniquePtr<FMOD::Sound> sample;
#elif defined(USE_SDL_MIXER)
		const SDLMixChunkUniquePtr sample;
#endif

		friend class C4AudioSystem;
	};
};
