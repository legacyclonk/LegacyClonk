/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
 * Copyright (c) 2017-2022, The LegacyClonk Team and contributors
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
#include <C4SoundSystem.h>

#include <C4Random.h>
#include <C4Object.h>
#include <C4Game.h>
#include <C4Log.h>
#include <C4Config.h>
#include <C4Application.h>
#include "Standard.h"

#include <algorithm>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

bool IsSoundPlaying(const char *const name, const C4Object *const obj)
{
	return Application.SoundSystem->FindInst(name, obj).has_value();
}

void SoundLevel(const char *const name, C4Object *const obj, const std::int32_t level)
{
	// Sound level zero? Stop
	if (level <= 0) { StopSoundEffect(name, obj); return; }
	// Set volume of existing instance or create new instance
	const auto it = Application.SoundSystem->FindInst(name, obj);
	if (it)
	{
		(**it).volume = level;
	}
	else
	{
		StartSoundEffect(name, true, level, obj);
	}
}

bool StartSoundEffect(const char *const name, const bool loop, const std::int32_t volume,
	C4Object *const obj, const std::int32_t falloffDistance)
{
	return Application.SoundSystem->NewInstance(name, loop, volume, 0, obj, falloffDistance) != nullptr;
}

void StartSoundEffectAt(const char *const name, const std::int32_t x, const std::int32_t y)
{
	std::int32_t volume, pan;
	Application.SoundSystem->GetVolumeByPos(x, y, volume, pan);
	Application.SoundSystem->NewInstance(name, false, volume, pan, nullptr, 0);
}

void StopSoundEffect(const char *const name, const C4Object *const obj)
{
	Application.SoundSystem->StopSoundEffect(name, obj);
}

C4SoundSystem::C4SoundSystem()
{
	// Load Sound.c4g
	C4Group soundFolder;
	if (soundFolder.Open(Config.AtExePath(C4CFN_Sound)))
	{
		LoadEffects(soundFolder);
	}
	else
	{
		Log(LoadResStr("IDS_PRC_NOSND"));
	}
}

void C4SoundSystem::ClearPointers(const C4Object *const obj)
{
	instances.remove_if([&](auto &inst) { return obj == inst.GetObj() && !inst.DetachObj(); });
}

void C4SoundSystem::Execute()
{
	instances.remove_if([](auto &inst) { return !inst.Execute(); });
}

void C4SoundSystem::LoadEffects(C4Group &group)
{
	if (!Application.AudioSystem) return;

	// Process segmented list of file types
	for (const auto fileType : { "*.wav", "*.ogg", "*.mp3" })
	{
		char filename[_MAX_FNAME + 1];
		// Search all sound files in group
		group.ResetSearch();
		while (group.FindNextEntry(fileType, filename))
		{
			// Load sample
			StdBuf buf;
			if (!group.LoadEntry(filename, buf)) continue;
			try
			{
				samples.insert_or_assign(filename, Sample{buf.getData(), buf.getSize()});
			}
			catch (const std::runtime_error &e)
			{
				LogF("WARNING: Could not load sound effect \"%s/%s\": %s", group.GetFullName().getData(), filename, e.what());
			}
		}
	}
}

bool C4SoundSystem::ToggleOnOff()
{
	auto &enabled = GetCfgSoundEnabled();
	return enabled = !enabled;
}

C4SoundSystem::Sample::Sample(const void *const buf, const std::size_t size)
	: sample{Application.AudioSystem->CreateSoundFile(buf, size)}, duration{sample->GetDuration()} {}

bool C4SoundSystem::Instance::DetachObj()
{
	// Stop if looping (would most likely loop forever)
	if (loop) return false;
	// Otherwise: set volume by last position
	const auto detachedObj = GetObj();
	obj.emplace<const ObjPos>(*detachedObj);
	GetVolumeByPos(detachedObj->x, detachedObj->y, volume, pan);

	// Do not stop instance
	return true;
}

bool C4SoundSystem::Instance::Execute(const bool justStarted)
{
	// Object deleted? Detach object and check if this would delete this instance
	const auto obj = GetObj();
	if (obj && !obj->Status && !DetachObj()) return false;

	// Remove instances that have stopped
	if (channel && !channel->IsPlaying())
	{
		if (loop && samples.size() > 1)
		{
			channel.reset();
			currentSample = NextSample();
		}
		else
		{
			return false;
		}
	}

	// Remove non-looping, inaudible sounds if half the time is up
	std::uint32_t pos = ~0;
	if (!loop && !channel && !justStarted)
	{
		pos = GetPlaybackPosition();
		if (pos > currentSample->get().duration / 2) return false;
	}

	// Muted: No need to calculate new volume
	if (!GetCfgSoundEnabled())
	{
		channel.reset();
		return true;
	}

	// Calculate volume and pan
	float vol = volume * Config.Sound.SoundVolume / 10000.0f, pan = this->pan / 100.0f;
	if (obj)
	{
		std::int32_t audibility = obj->GetAudibility();
		// apply custom falloff distance
		if (falloffDistance != 0)
		{
			audibility = std::clamp<int32_t>(100 + (audibility - 100) * AudibilityRadius / falloffDistance, 0, 100);
		}
		vol *= audibility / 100.0f;
		pan += obj->GetAudiblePan() / 100.0f;
	}

	// Sound off? Release channel to make it available for other instances.
	if (vol <= 0.0f)
	{
		channel.reset();
		return true;
	}

	try
	{
		// Create/restore channel if necessary
		const auto createChannel = !channel;
		if (createChannel)
		{
			channel.reset(Application.AudioSystem->CreateSoundChannel(currentSample->get().sample.get(), loop && samples.size() == 1));
			if (!justStarted)
			{
				if (pos == ~0) pos = GetPlaybackPosition();
				channel->SetPosition(pos);
			}
		}

		// Update volume
		channel->SetVolumeAndPan(vol, pan);
		if (createChannel) channel->Unpause();
	}
	catch (const std::runtime_error &e)
	{
		LogSilent(e.what());
		return false;
	}

	return true;
}

C4Object *C4SoundSystem::Instance::GetObj() const
{
	const auto ptr = std::get_if<C4Object *>(&obj);
	return ptr ? *ptr : nullptr;
}

std::uint32_t C4SoundSystem::Instance::GetPlaybackPosition() const
{
	return currentSample->get().duration ? std::chrono::duration_cast<std::chrono::milliseconds>(
								 std::chrono::steady_clock::now() - startTime).count() % currentSample->get().duration
						   : 0;
}

bool C4SoundSystem::Instance::IsNear(const C4Object &obj2) const
{
	// Attached to object?
	if (const auto objAsObject = std::get_if<C4Object *>(&obj); objAsObject && *objAsObject)
	{
		const auto x = (**objAsObject).x;
		const auto y = (**objAsObject).y;
		return (x - obj2.x) * (x - obj2.x) + (y - obj2.y) * (y - obj2.y) <=
		NearSoundRadius * NearSoundRadius;
	}

	// Global or was attached to deleted object
	// Deleted objects' sounds could be considered near,
	//  but "original" Clonk Rage behavior is to not honor them.
	// This must not change, otherwise it would break some scenarios / packs (e.g. CMC)
	return false;
}

auto C4SoundSystem::Instance::NextSample() -> decltype(samples)::iterator
{
	return std::begin(samples) + SafeRandom(samples.size());
}

bool C4SoundSystem::CaseInsensitiveLess::operator()(const std::string &first, const std::string &second) const
{
	return stricmp(first.c_str(), second.c_str()) < 0;
}

std::optional<std::list<C4SoundSystem::Instance>::iterator> C4SoundSystem::FindInst(const char *wildcard, const C4Object *const obj)
{
	const auto wildcardStr = PrepareFilename(wildcard);
	wildcard = wildcardStr.c_str();

	const auto it = std::find_if(instances.begin(), instances.end(), [obj, wildcard](const auto &inst)
	{
		return inst.GetObj() == obj && WildcardMatch(inst.name.c_str(), wildcard);
	});

	if (it != instances.end())
	{
		return it;
	}

	return std::nullopt;
}

bool &C4SoundSystem::GetCfgSoundEnabled()
{
	return Game.IsRunning ? Config.Sound.RXSound : Config.Sound.FESamples;
}

void C4SoundSystem::GetVolumeByPos(std::int32_t x, std::int32_t y,
	std::int32_t &volume, std::int32_t &pan)
{
	volume = Game.GraphicsSystem.GetAudibility(x, y, &pan);
}

auto C4SoundSystem::NewInstance(const char *filename, const bool loop,
	const std::int32_t volume, const std::int32_t pan, C4Object *const obj,
	const std::int32_t falloffDistance) -> Instance *
{
	if (!Application.AudioSystem) return nullptr;

	const auto filenameStr = PrepareFilename(filename);
	filename = filenameStr.c_str();

	std::vector<std::reference_wrapper<Sample>> matchingSamples;

	// Search for matching file if name contains no wildcard
	if (filenameStr.find('?') == std::string::npos)
	{
		const auto it = samples.find(filename);
		// File not found
		if (it == samples.end()) return nullptr;
		// Success: Found the file
		matchingSamples = {it->second};
	}
	// Randomly select any matching file if name contains wildcard
	else
	{
		for (auto &[name, sample] : samples)
		{
			if (WildcardMatch(filename, name.c_str()))
			{
				matchingSamples.push_back(sample);
			}
		}
		// File not found
		if (matchingSamples.empty()) return nullptr;
	}

	// Too many instances?
	std::unordered_map<Sample *, std::size_t> counter;

	for (auto &instance : instances)
	{
		for (auto &sample : instance.samples)
		{
			for (auto &matchingSample : matchingSamples)
			{
				if (&sample.get() == &matchingSample.get())
				{
					if (++counter[&sample.get()] > MaxSoundInstances)
					{
						return nullptr;
					}

					if (obj)
					{
						if (instance.IsNear(*obj))
						{
							return nullptr;
						}

					}
					else if (!instance.GetObj())
					{
						return nullptr;
					}
				}
			}
		}
	}
	// Create instance
	auto &inst = instances.emplace_back(filename, std::move(matchingSamples), loop, volume, obj, falloffDistance);
	if (!inst.Execute(true))
	{
		instances.pop_back();
		return nullptr;
	}

	return &inst;
}

void C4SoundSystem::StopSoundEffect(const char *wildcard, const C4Object *obj)
{
	instances.remove_if([obj, wildcard](const auto &inst)
	{
		return inst.GetObj() == obj && WildcardMatch(inst.name.c_str(), wildcard);
	});
}

std::string C4SoundSystem::PrepareFilename(const char *const filename)
{
	auto result = *GetExtension(filename) ?
		filename : std::string{filename} + ".wav";
	std::replace(result.begin(), result.end(), '*', '?');
	return result;
}
