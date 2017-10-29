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

#include <C4Application.h>

#include <algorithm>
#include <stdexcept>
#include <string>

C4AudioSystem::C4AudioSystem(const int maxChannels)
{
	// Create system object
	FMOD::System *createdSystem;
	ThrowIfFailed("System_Create", FMOD::System_Create(&createdSystem));
	FmodUniquePtr<FMOD::System> system{createdSystem, ReleaseFmodObj<FMOD::System>};
	// Check FMOD version
	unsigned int version;
	ThrowIfFailed("getVersion", system->getVersion(&version));
	if (version < FMOD_VERSION)
	{
		throw std::runtime_error(std::string() +
			"FMOD: You are using the wrong DLL version! You should be using " +
			std::to_string(FMOD_VERSION & 0xffff0000) + "." +
			std::to_string(FMOD_VERSION & 0xff00) + "." +
			std::to_string(FMOD_VERSION & 0xff));
	}
	// Initialize system object and sound device
	ThrowIfFailed("init", system->init(maxChannels + 1, FMOD_INIT_NORMAL, nullptr));
	// ok
	this->system.reset(system.release());
}

auto C4AudioSystem::FmodCreateSound(const void *const buf, const std::size_t size,
	const bool decompress) -> FmodUniquePtr<FMOD::Sound>
{
	FMOD_CREATESOUNDEXINFO info{};
	info.cbsize = sizeof(info);
	info.length = size;
	FMOD::Sound *sample;
	const FMOD_MODE mode = FMOD_DEFAULT | FMOD_LOOP_NORMAL | FMOD_OPENMEMORY |
		(decompress ? FMOD_CREATESAMPLE : FMOD_CREATESTREAM);
	ThrowIfFailed("createSound",
		system->createSound(static_cast<const char *>(buf), mode, &info, &sample));
	return {sample, ReleaseFmodObj<FMOD::Sound>};
}

auto C4AudioSystem::FmodPlaySound(FMOD::Sound &sample, bool loop, const bool maxPriority) -> FmodChannelUniquePtr
{
	// Play sound
	FMOD::Channel *returnedChannel;
	ThrowIfFailed("playSound", system->playSound(&sample, nullptr, true, &returnedChannel));
	FmodChannelUniquePtr channel{returnedChannel, StopFmodChannel};
	// Disable loop if not requested
	if (!loop)
	{
		ThrowIfFailed("setLoopCount", channel->setLoopCount(0));
	}
	// Use maximum priority for channel if requested
	if (maxPriority)
	{
		ThrowIfFailed("setPriority", channel->setPriority(0));
	}

	return channel;
}

void C4AudioSystem::ThrowIfFailed(const char *const funcName, const FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
		throw std::runtime_error(std::string{"FMOD: "} +
			funcName + " failed: " + FMOD_ErrorString(result));
	}
}

void C4AudioSystem::FadeOutMusic(const std::int32_t ms)
{
	// Get sample rate
	int sampleRate;
	ThrowIfFailed("getSoftwareFormat",
		Application.AudioSystem->system->getSoftwareFormat(&sampleRate, nullptr, nullptr));

	// Get current DSP clock and DSP clock where volume must be zero
	unsigned long long currentDspClock;
	ThrowIfFailed("getDSPClock", musicChannel->getDSPClock(nullptr, &currentDspClock));
	unsigned long long endDspClock = currentDspClock + sampleRate * ms / 1000;

	// Fade volume to zero
	ThrowIfFailed("addFadePoint", musicChannel->addFadePoint(currentDspClock, 1.0f));
	ThrowIfFailed("addFadePoint", musicChannel->addFadePoint(endDspClock, 0.0f));
	ThrowIfFailed("setDelay", musicChannel->setDelay(0, endDspClock));
}

bool C4AudioSystem::IsMusicPlaying() const
{
	if (!musicChannel) return false;
	bool isPlaying;
	ThrowIfFailed("isPlaying", musicChannel->isPlaying(&isPlaying));
	return isPlaying;
}

void C4AudioSystem::PlayMusic(const MusicFile &music, const bool loop)
{
	musicChannel = Application.AudioSystem->FmodPlaySound(*music.sample, loop, true);
}

void C4AudioSystem::SetMusicVolume(const float volume)
{
	ThrowIfFailed("setVolume", musicChannel->setVolume(volume));
}

void C4AudioSystem::StopMusic()
{
	musicChannel.reset();
}

void C4AudioSystem::UnpauseMusic()
{
	ThrowIfFailed("setPaused", musicChannel->setPaused(false));
}

C4AudioSystem::MusicFile::MusicFile(const void *const buf, const std::size_t size)
	: sample{Application.AudioSystem->FmodCreateSound(buf, size, false)} {}

C4AudioSystem::SoundChannel::SoundChannel(const SoundFile &sound, const bool loop)
	: channel{Application.AudioSystem->FmodPlaySound(*sound.sample, loop, false)} {}

bool C4AudioSystem::SoundChannel::IsPlaying() const
{
	bool isPlaying;
	ThrowIfFailed("isPlaying", channel->isPlaying(&isPlaying));
	return isPlaying;
}

void C4AudioSystem::SoundChannel::SetPosition(const std::uint32_t ms)
{
	ThrowIfFailed("setPosition", channel->setPosition(static_cast<unsigned int>(ms), FMOD_TIMEUNIT_MS));
}

void C4AudioSystem::SoundChannel::SetVolumeAndPan(const float volume, const float pan)
{
	ThrowIfFailed("setVolume", channel->setVolume(volume));
	ThrowIfFailed("setPan", channel->setPan(pan));
}

void C4AudioSystem::SoundChannel::Unpause()
{
	ThrowIfFailed("setPaused", channel->setPaused(false));
}

C4AudioSystem::SoundFile::SoundFile(const void *const buf, const std::size_t size)
	: sample{Application.AudioSystem->FmodCreateSound(buf, size, true)} {}

std::uint32_t C4AudioSystem::SoundFile::GetDuration() const
{
	unsigned int duration;
	ThrowIfFailed("getLength", sample->getLength(&duration, FMOD_TIMEUNIT_MS));
	return duration;
}
