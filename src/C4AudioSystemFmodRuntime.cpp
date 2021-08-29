/*
 * LegacyClonk
 *
 * Copyright (c) 2020, The LegacyClonk Team and contributors
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

#include "C4AudioSystemFmodRuntime.h"

#include "C4Application.h"

#include "StdHelpers.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <variant>

#include <utility>

// order matters
#include <windows.h>
#include <libloaderapi.h>

#ifndef _WIN64
#define F_API __stdcall
#else
#define F_API
#endif

static std::string ErrorString()
{
	const DWORD errorCode{GetLastError()};
	LPSTR buffer{nullptr};
	if (FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr,
				errorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPSTR>(&buffer),
				0,
				nullptr
				))
	{
		std::string ret{buffer};
		LocalFree(buffer);
		return ret;
	}
	else
	{
		return FormatString("Code: %u (FormatMessage failed with error %u)", errorCode, GetLastError()).getData();
	}
}

class Library
{
	HMODULE lib;

public:
	Library(const std::string &path);
	~Library();

	template<typename T>
	T LoadSymbol(const std::string &name)
	{
		return reinterpret_cast<T>(LoadSymbolInternal(name));
	}

private:
	void *LoadSymbolInternal(const std::string &name);

protected:
	template <typename T>
	class Symbol
	{
		const T symbol;

	public:
		Symbol(Library *lib, const std::string &name) : symbol{lib->LoadSymbol<T>(name)} {}

		T Get() const { return symbol; }
		operator T() const { return Get(); }
	};

	template <typename T>
	class Func;

	template <typename Ret, typename... Args>
	class Func<Ret(Args...)> : public Symbol<Ret(F_API *)(Args...)>
	{
		using FuncType = Ret(F_API *)(Args...);

#ifndef _WIN64
		static constexpr auto ArgsSize = ([]
		{
			// rounded up to multiple of 4
			return (((sizeof(Args) + 3) & ~3) + ... + 0);
		})();
#endif

	public:
		Func(Library *lib, const std::string &name) :
#ifndef _WIN64
			Symbol<FuncType>{lib, "_" + name + "@" + std::to_string(ArgsSize)}
#else
			Symbol<FuncType>{lib, name}
#endif
			{}

		Ret operator()(Args &&... args) const
		{
			return this->Get()(std::forward<Args>(args)...);
		}
	};
};

Library::Library(const std::string &path) : lib{LoadLibraryA(path.c_str())}
{
	if (!lib)
	{
		throw std::runtime_error{"Could not load library '" + path + "': " + ErrorString()};
	}
}

Library::~Library()
{
	if (lib)
	{
		FreeLibrary(lib);
	}
}

void *Library::LoadSymbolInternal(const std::string &name)
{
	if (const auto symbol = GetProcAddress(lib, name.c_str()); !symbol)
	{
		throw std::runtime_error{"Could not get symbol address for '" + name + "': " + ErrorString()};
	}
	else
	{
		return reinterpret_cast<void *>(symbol);
	}
}

struct FmodRuntime : private Library
{
	static constexpr auto FMOD_VERSION = 3.75f;
	static constexpr auto FSOUND_FREE = -1;
	static constexpr auto FSOUND_UNMANAGED = -2;
	static constexpr auto FSOUND_ALL = -3;

	static constexpr auto FSOUND_LOOP_OFF = 0x1;
	static constexpr auto FSOUND_LOOP_NORMAL = 0x2;
	static constexpr auto FSOUND_2D = 0x2000;
	static constexpr auto FSOUND_LOADMEMORY = 0x8000;
	static constexpr auto FSOUND_16BITS = 0x10;
	static constexpr auto FSOUND_MONO = 0x20;
	static constexpr auto FSOUND_SIGNED = 0x100;

	static constexpr auto FSOUND_NORMAL = FSOUND_16BITS | FSOUND_SIGNED | FSOUND_MONO;

	struct FSOUND_SAMPLE;
	struct FSOUND_STREAM;
	struct FMUSIC_MODULE;

	enum FSOUND_MIXERTYPES
	{
		FSOUND_MIXER_AUTODETECT,
		FSOUND_MIXER_BLENDMODE,
		FSOUND_MIXER_MMXP5,
		FSOUND_MIXER_MMXP6,
		FSOUND_MIXER_QUALITY_AUTODETECT,
		FSOUND_MIXER_QUALITY_FPU,
		FSOUND_MIXER_QUALITY_MMXP5,
		FSOUND_MIXER_QUALITY_MMXP6,
		FSOUND_MIXER_MONO,
		FSOUND_MIXER_QUALITY_MONO,
		FSOUND_MIXER_MAX
	};

	enum FSOUND_OUTPUTTYPES
	{
		FSOUND_OUTPUT_NOSOUND,
		FSOUND_OUTPUT_WINMM,
		FSOUND_OUTPUT_DSOUND,
		FSOUND_OUTPUT_A3D,
		FSOUND_OUTPUT_OSS,
		FSOUND_OUTPUT_ESD,
		FSOUND_OUTPUT_ALSA,
		FSOUND_OUTPUT_ASIO,
		FSOUND_OUTPUT_XBOX,
		FSOUND_OUTPUT_PS2,
		FSOUND_OUTPUT_MAC,
		FSOUND_OUTPUT_GC,
		FSOUND_OUTPUT_PSP,
		FSOUND_OUTPUT_NOSOUND_NONREALTIME
	};

	Func<signed char(int)> FSOUND_SetOutput;
	Func<signed char(int)> FSOUND_SetMixer;
	Func<signed char(void *)> FSOUND_SetHWND;
	Func<signed char(int, int, unsigned int)> FSOUND_Init;
	Func<void()> FSOUND_Close;
	Func<int()> FSOUND_GetError;
	Func<float()> FSOUND_GetVersion;
	Func<FSOUND_SAMPLE *(int, const char *, unsigned int, int, int)> FSOUND_Sample_Load;
	Func<void(FSOUND_SAMPLE *)> FSOUND_Sample_Free;
	Func<unsigned int(FSOUND_SAMPLE *)> FSOUND_Sample_GetLength;
	Func<signed char(FSOUND_SAMPLE *, int *, int *, int *, int *)> FSOUND_Sample_GetDefaults;
	Func<int(int, FSOUND_SAMPLE *)> FSOUND_PlaySound;
	Func<signed char(int)> FSOUND_StopSound;
	Func<signed char(int, int)> FSOUND_SetVolume;
	Func<signed char(int, int)> FSOUND_SetPan;
	Func<signed char(int, int)> FSOUND_SetPriority;
	Func<signed char(int, signed char)> FSOUND_SetPaused;
	Func<signed char(int, unsigned int)> FSOUND_SetLoopMode;
	Func<signed char(int, unsigned int)> FSOUND_SetCurrentPosition;
	Func<signed char(int)> FSOUND_IsPlaying;
	Func<FSOUND_SAMPLE *(int)> FSOUND_GetCurrentSample;
	Func<FSOUND_STREAM *(const char *, unsigned int, int, int)> FSOUND_Stream_Open;
	Func<signed char(FSOUND_STREAM *)> FSOUND_Stream_Close;
	Func<int(int, FSOUND_STREAM *)> FSOUND_Stream_Play;
	Func<signed char(FSOUND_STREAM *)> FSOUND_Stream_Stop;
	Func<signed char(FSOUND_STREAM *, int)> FSOUND_Stream_SetLoopCount;
	Func<FMUSIC_MODULE *(const char *, int, int, unsigned int, const int *, int)> FMUSIC_LoadSongEx;
	Func<signed char(FMUSIC_MODULE *)> FMUSIC_FreeSong;
	Func<signed char(FMUSIC_MODULE *)> FMUSIC_PlaySong;
	Func<signed char(FMUSIC_MODULE *)> FMUSIC_StopSong;
	Func<signed char(FMUSIC_MODULE *, signed char)> FMUSIC_SetLooping;
	Func<signed char(FMUSIC_MODULE *, signed char)> FMUSIC_SetPaused;
	Func<signed char(FMUSIC_MODULE *, int)> FMUSIC_SetMasterVolume;
	Func<signed char(FMUSIC_MODULE *)> FMUSIC_IsPlaying;

	FmodRuntime() : Library
		{
#ifdef _WIN64
			"fmod64.dll"
#else
			"fmod.dll"
#endif
		},
		FSOUND_SetOutput{this, "FSOUND_SetOutput"},
		FSOUND_SetMixer{this, "FSOUND_SetMixer"},
		FSOUND_SetHWND{this, "FSOUND_SetHWND"},
		FSOUND_Init{this, "FSOUND_Init"},
		FSOUND_Close{this, "FSOUND_Close"},
		FSOUND_GetError{this, "FSOUND_GetError"},
		FSOUND_GetVersion{this, "FSOUND_GetVersion"},
		FSOUND_Sample_Load{this, "FSOUND_Sample_Load"},
		FSOUND_Sample_Free{this, "FSOUND_Sample_Free"},
		FSOUND_Sample_GetLength{this, "FSOUND_Sample_GetLength"},
		FSOUND_Sample_GetDefaults{this, "FSOUND_Sample_GetDefaults"},
		FSOUND_PlaySound{this, "FSOUND_PlaySound"},
		FSOUND_StopSound{this, "FSOUND_StopSound"},
		FSOUND_SetVolume{this, "FSOUND_SetVolume"},
		FSOUND_SetPan{this, "FSOUND_SetPan"},
		FSOUND_SetPriority{this, "FSOUND_SetPriority"},
		FSOUND_SetPaused{this, "FSOUND_SetPaused"},
		FSOUND_SetLoopMode{this, "FSOUND_SetLoopMode"},
		FSOUND_SetCurrentPosition{this, "FSOUND_SetCurrentPosition"},
		FSOUND_IsPlaying{this, "FSOUND_IsPlaying"},
		FSOUND_GetCurrentSample{this, "FSOUND_GetCurrentSample"},
		FSOUND_Stream_Open{this, "FSOUND_Stream_Open"},
		FSOUND_Stream_Close{this, "FSOUND_Stream_Close"},
		FSOUND_Stream_Play{this, "FSOUND_Stream_Play"},
		FSOUND_Stream_Stop{this, "FSOUND_Stream_Stop"},
		FSOUND_Stream_SetLoopCount{this, "FSOUND_Stream_SetLoopCount"},
		FMUSIC_LoadSongEx{this, "FMUSIC_LoadSongEx"},
		FMUSIC_FreeSong{this, "FMUSIC_FreeSong"},
		FMUSIC_PlaySong{this, "FMUSIC_PlaySong"},
		FMUSIC_StopSong{this, "FMUSIC_StopSong"},
		FMUSIC_SetLooping{this, "FMUSIC_SetLooping"},
		FMUSIC_SetPaused{this, "FMUSIC_SetPaused"},
		FMUSIC_SetMasterVolume{this, "FMUSIC_SetMasterVolume"},
		FMUSIC_IsPlaying{this, "FMUSIC_IsPlaying"}
	{}

	enum FMOD_ERRORS
	{
		FMOD_ERR_NONE,
		FMOD_ERR_BUSY,
		FMOD_ERR_UNINITIALIZED,
		FMOD_ERR_INIT,
		FMOD_ERR_ALLOCATED,
		FMOD_ERR_PLAY,
		FMOD_ERR_OUTPUT_FORMAT,
		FMOD_ERR_COOPERATIVELEVEL,
		FMOD_ERR_CREATEBUFFER,
		FMOD_ERR_FILE_NOTFOUND,
		FMOD_ERR_FILE_FORMAT,
		FMOD_ERR_FILE_BAD,
		FMOD_ERR_MEMORY,
		FMOD_ERR_VERSION,
		FMOD_ERR_INVALID_PARAM,
		FMOD_ERR_NO_EAX,
		FMOD_ERR_CHANNEL_ALLOC,
		FMOD_ERR_RECORD,
		FMOD_ERR_MEDIAPLAYER,
		FMOD_ERR_CDDEVICE
	};

	static const char *FMOD_ErrorString(int errcode)
	{
		switch (errcode)
		{
			case FMOD_ERR_NONE:				return "No errors";
			case FMOD_ERR_BUSY:				return "Cannot call this command after FSOUND_Init.  Call FSOUND_Close first.";
			case FMOD_ERR_UNINITIALIZED:	return "This command failed because FSOUND_Init was not called";
			case FMOD_ERR_PLAY:				return "Playing the sound failed.";
			case FMOD_ERR_INIT:				return "Error initializing output device.";
			case FMOD_ERR_ALLOCATED:		return "The output device is already in use and cannot be reused.";
			case FMOD_ERR_OUTPUT_FORMAT:	return "Soundcard does not support the features needed for this soundsystem (16bit stereo output)";
			case FMOD_ERR_COOPERATIVELEVEL:	return "Error setting cooperative level for hardware.";
			case FMOD_ERR_CREATEBUFFER:		return "Error creating hardware sound buffer.";
			case FMOD_ERR_FILE_NOTFOUND:	return "File not found";
			case FMOD_ERR_FILE_FORMAT:		return "Unknown file format";
			case FMOD_ERR_FILE_BAD:			return "Error loading file";
			case FMOD_ERR_MEMORY:			return "Not enough memory ";
			case FMOD_ERR_VERSION:			return "The version number of this file format is not supported";
			case FMOD_ERR_INVALID_PARAM:	return "An invalid parameter was passed to this function";
			case FMOD_ERR_NO_EAX:			return "Tried to use an EAX command on a non EAX enabled channel or output.";
			case FMOD_ERR_CHANNEL_ALLOC:	return "Failed to allocate a new channel";
			case FMOD_ERR_RECORD:			return "Recording not supported on this device";
			case FMOD_ERR_MEDIAPLAYER:		return "Required Mediaplayer codec is not installed";
		};
		return "Unknown error";
	}
};

class C4AudioSystemFmodRuntime : public C4AudioSystem, public FmodRuntime
{
public:
	explicit C4AudioSystemFmodRuntime(int maxChannels);
	~C4AudioSystemFmodRuntime();

	virtual void FadeOutMusic(std::int32_t ms) override;
	virtual bool IsMusicPlaying() const override;
	// Plays music file. If supported by audio library, playback starts paused.
	virtual void PlayMusic(const MusicFile *const music, bool loop) override;
	// Range: 0.0f through 1.0f
	virtual void SetMusicVolume(float volume) override;
	virtual void StopMusic() override;
	virtual void UnpauseMusic() override;

	class FmodMusicFile : public MusicFile
	{
		std::variant<FMUSIC_MODULE *, FSOUND_STREAM *> music;
		mutable int channel;
		mutable std::thread checkModuleThread; // necessary due to Fmod's beloved timeout when checking IsPlaying on a midi "module"
		mutable std::atomic<bool> modulePlaying{false};
		C4AudioSystemFmodRuntime *audioSystem;

	public:
		FmodMusicFile(C4AudioSystemFmodRuntime *audioSystem, const void *buf, std::size_t size);
		virtual ~FmodMusicFile();
		bool IsPlaying() const;
		void SetPaused(bool paused) const;
		void Stop() const;
		void SetVolume(float volume) const;
		void Play(bool loop) const;
	};

	virtual MusicFile *CreateMusicFile(const void *buf, std::size_t size) override;

	class FmodSoundFile;
	class FmodSoundChannel : public SoundChannel
	{
		int channel;
		const FmodSoundFile *sound;
		C4AudioSystemFmodRuntime *audioSystem;

	public:
		FmodSoundChannel(C4AudioSystemFmodRuntime *audioSystem, const SoundFile *sound, bool loop);
		~FmodSoundChannel();

		virtual bool IsPlaying() const override;
		virtual void SetPosition(std::uint32_t ms) override;
		// volume range: 0.0f through 1.0f; pan range: -1.0f through 1.0f
		virtual void SetVolumeAndPan(float volume, float pan) override;
		virtual void Unpause() override;
	};

	virtual SoundChannel *CreateSoundChannel(const SoundFile *const sound, bool loop) override;

	class FmodSoundFile : public SoundFile
	{
		std::uint32_t duration;
		C4AudioSystemFmodRuntime *audioSystem;

	public:
		FSOUND_SAMPLE *sample;
		int sampleRate;

		FmodSoundFile(C4AudioSystemFmodRuntime *audioSystem, const void *buf, std::size_t size);
		~FmodSoundFile();

		virtual std::uint32_t GetDuration() const;
	};

	virtual SoundFile *CreateSoundFile(const void *buf, std::size_t size) override;

private:
	const FmodMusicFile *currentMusicFile{};
};

C4AudioSystemFmodRuntime::C4AudioSystemFmodRuntime(int maxChannels)
{
	if (FSOUND_GetVersion() < FMOD_VERSION)
	{
		throw std::runtime_error{"Fmod: You are using the wrong DLL version! You should be using " + std::to_string(FMOD_VERSION) + "."};
	}

	FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND);
	FSOUND_SetHWND(Application.pWindow->hWindow);
	FSOUND_SetMixer(FSOUND_MIXER_QUALITY_AUTODETECT);

	if (!FSOUND_Init(44100, maxChannels, 0))
	{
		throw std::runtime_error{std::string{"FMod: "} + FMOD_ErrorString(FSOUND_GetError())};
	}
}

C4AudioSystemFmodRuntime::~C4AudioSystemFmodRuntime()
{
	FSOUND_StopSound(FSOUND_ALL); // to prevent some hangs in Fmod
	FSOUND_Close();
}

void C4AudioSystemFmodRuntime::FadeOutMusic(std::int32_t)
{
	StopMusic();
}

bool C4AudioSystemFmodRuntime::IsMusicPlaying() const
{
	return currentMusicFile && currentMusicFile->IsPlaying();
}

void C4AudioSystemFmodRuntime::PlayMusic(const MusicFile *const music, bool loop)
{
	currentMusicFile = static_cast<const FmodMusicFile *>(music);
	currentMusicFile->Play(loop);
}

void C4AudioSystemFmodRuntime::SetMusicVolume(float volume)
{
	currentMusicFile->SetVolume(volume);
}

void C4AudioSystemFmodRuntime::StopMusic()
{
	if (currentMusicFile)
	{
		currentMusicFile->Stop();
	}
}

void C4AudioSystemFmodRuntime::UnpauseMusic()
{
	currentMusicFile->SetPaused(false);
}

C4AudioSystemFmodRuntime::FmodMusicFile::FmodMusicFile(C4AudioSystemFmodRuntime *audioSystem, const void *buf, std::size_t size) : audioSystem{audioSystem}
{
	static constexpr auto flags = FSOUND_LOADMEMORY | FSOUND_NORMAL | FSOUND_2D | FSOUND_LOOP_NORMAL;
	if (const auto stream = audioSystem->FSOUND_Stream_Open(static_cast<const char *>(buf), flags, 0, size); stream)
	{
		audioSystem->FSOUND_Stream_SetLoopCount(stream, 0);
		music = stream;
	}
	else
	{
		const auto streamError = audioSystem->FSOUND_GetError();
		const auto song = audioSystem->FMUSIC_LoadSongEx(static_cast<const char *>(buf), 0, size, flags, nullptr, 0);
		if (!song)
		{
			throw std::runtime_error{std::string{"Fmod: Loading Music failed: As Stream: " } + FMOD_ErrorString(streamError) + std::string{"; As Music: "} + FMOD_ErrorString(audioSystem->FSOUND_GetError())};
		}

		audioSystem->FMUSIC_SetLooping(song, false);

		music = song;
	}
}

C4AudioSystemFmodRuntime::FmodMusicFile::~FmodMusicFile()
{
	Stop();
	if (checkModuleThread.joinable())
	{
		checkModuleThread.join();
	}
	std::visit(StdOverloadedCallable
	{
		audioSystem->FMUSIC_FreeSong,
		audioSystem->FSOUND_Stream_Close
	}, music);
	if (audioSystem->currentMusicFile == this)
	{
		audioSystem->currentMusicFile = nullptr;
	}
}

bool C4AudioSystemFmodRuntime::FmodMusicFile::IsPlaying() const
{
	return std::visit(StdOverloadedCallable
	{
		[this](FMUSIC_MODULE *) -> bool
		{
			return modulePlaying;
		},
		[this](FSOUND_STREAM *stream) -> bool
		{
			return audioSystem->FSOUND_IsPlaying(channel);
		}
	}, music);
}

void C4AudioSystemFmodRuntime::FmodMusicFile::SetPaused(bool paused) const
{
	std::visit(StdOverloadedCallable
	{
		[this, paused](FMUSIC_MODULE *song)
		{
			audioSystem->FMUSIC_SetPaused(song, paused);
		},
		[this, paused](FSOUND_STREAM *stream)
		{
			audioSystem->FSOUND_SetPaused(channel, paused);
		}
	}, music);
}

void C4AudioSystemFmodRuntime::FmodMusicFile::Stop() const
{
	std::visit(StdOverloadedCallable
	{
		audioSystem->FMUSIC_StopSong,
		audioSystem->FSOUND_Stream_Stop
	}, music);
}

void C4AudioSystemFmodRuntime::FmodMusicFile::SetVolume(float volume) const
{
	std::visit(StdOverloadedCallable
	{
		[this, volume](FMUSIC_MODULE *song)
		{
			audioSystem->FMUSIC_SetMasterVolume(song, static_cast<int>(std::lrint(volume * 255)));
		},
		[this, volume](FSOUND_STREAM *stream)
		{
			audioSystem->FSOUND_SetVolume(channel, static_cast<int>(std::lrint(volume * 255)));
		}
	}, music);
}

void C4AudioSystemFmodRuntime::FmodMusicFile::Play(bool loop) const
{
	std::visit(StdOverloadedCallable
	{
		[this, loop](FMUSIC_MODULE *song)
		{
			modulePlaying = true;
			audioSystem->FMUSIC_SetLooping(song, loop);
			audioSystem->FMUSIC_PlaySong(song);
			checkModuleThread = std::thread{[this, song]()
				{
					while (audioSystem->FMUSIC_IsPlaying(song))
					{
						using namespace std::chrono_literals;
						std::this_thread::sleep_for(10ms);
					}
					modulePlaying = false;
				}
			};
		},
		[this, loop](FSOUND_STREAM *stream)
		{
			audioSystem->FSOUND_Stream_SetLoopCount(stream, loop ? -1 : 0);
			channel = audioSystem->FSOUND_Stream_Play(FSOUND_FREE, stream);
			if (channel == -1 || !audioSystem->FSOUND_SetPriority(channel, 255))
			{
				throw std::runtime_error{std::string{"Fmod: "} + FMOD_ErrorString(audioSystem->FSOUND_GetError())};
			}
		}
	}, music);
	SetPaused(true);
}

C4AudioSystemFmodRuntime::MusicFile *C4AudioSystemFmodRuntime::CreateMusicFile(const void *buf, std::size_t size)
{
	return new FmodMusicFile{this, buf, size};
}

C4AudioSystemFmodRuntime::FmodSoundChannel::FmodSoundChannel(C4AudioSystemFmodRuntime *audioSystem, const SoundFile *const sound_, bool loop) : audioSystem{audioSystem}
{
	sound = static_cast<const FmodSoundFile *>(sound_);
	channel = audioSystem->FSOUND_PlaySound(FSOUND_FREE, sound->sample);
	if (channel == -1 || !audioSystem->FSOUND_SetLoopMode(channel, loop ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF) || !audioSystem->FSOUND_SetPaused(channel, true))
	{
		throw std::runtime_error{std::string{"Fmod: "} + FMOD_ErrorString(audioSystem->FSOUND_GetError())};
	}
}

C4AudioSystemFmodRuntime::FmodSoundChannel::~FmodSoundChannel()
{
	audioSystem->FSOUND_StopSound(channel);
}

bool C4AudioSystemFmodRuntime::FmodSoundChannel::IsPlaying() const
{
	return audioSystem->FSOUND_GetCurrentSample(channel) == sound->sample;
}

void C4AudioSystemFmodRuntime::FmodSoundChannel::SetPosition(std::uint32_t ms)
{
	audioSystem->FSOUND_SetCurrentPosition(channel, ms / 10 * sound->sampleRate / 100);
}

void C4AudioSystemFmodRuntime::FmodSoundChannel::SetVolumeAndPan(float volume, float pan)
{
	audioSystem->FSOUND_SetVolume(channel, static_cast<int>(std::lrint(volume * 255)));
	audioSystem->FSOUND_SetPan(channel, static_cast<int>(std::lrint((0.5f * pan + 0.5f) * 255)));
}

void C4AudioSystemFmodRuntime::FmodSoundChannel::Unpause()
{
	if (!audioSystem->FSOUND_SetPaused(channel, false))
	{
		throw std::runtime_error{std::string{"Fmod: "} + FMOD_ErrorString(audioSystem->FSOUND_GetError())};
	}
}

C4AudioSystemFmodRuntime::SoundChannel *C4AudioSystemFmodRuntime::CreateSoundChannel(const SoundFile *const sound, bool loop)
{
	return new FmodSoundChannel{this, sound, loop};
}

C4AudioSystemFmodRuntime::FmodSoundFile::FmodSoundFile(C4AudioSystemFmodRuntime *audioSystem, const void *buf, std::size_t size) : audioSystem{audioSystem}
{
	sample = audioSystem->FSOUND_Sample_Load(FSOUND_UNMANAGED, static_cast<const char *>(buf), FSOUND_LOADMEMORY | FSOUND_NORMAL | FSOUND_2D, 0, size);

	if (!sample)
	{
		throw std::runtime_error{std::string{"Fmod: "} + FMOD_ErrorString(audioSystem->FSOUND_GetError())};
	}

	const auto samples = audioSystem->FSOUND_Sample_GetLength(sample);
	if (!audioSystem->FSOUND_Sample_GetDefaults(sample, &sampleRate, nullptr, nullptr, nullptr))
	{
		throw std::runtime_error{std::string{"Fmod: "} + FMOD_ErrorString(audioSystem->FSOUND_GetError())};
	}

	duration = samples * 10 / (sampleRate / 100);
}

C4AudioSystemFmodRuntime::FmodSoundFile::~FmodSoundFile()
{
	audioSystem->FSOUND_Sample_Free(sample);
}

std::uint32_t C4AudioSystemFmodRuntime::FmodSoundFile::GetDuration() const
{
	return duration;
}

C4AudioSystemFmodRuntime::SoundFile *C4AudioSystemFmodRuntime::CreateSoundFile(const void *buf, std::size_t size)
{
	return new FmodSoundFile{this, buf, size};
}

C4AudioSystem *CreateC4AudioSystemFmodRuntime(int maxChannels)
{
	return new C4AudioSystemFmodRuntime{maxChannels};
}
