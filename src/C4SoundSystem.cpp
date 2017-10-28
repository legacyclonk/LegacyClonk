/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

#include <C4Include.h>
#include <C4SoundSystem.h>

#include <C4Random.h>
#include <C4Object.h>
#include <C4Game.h>
#include <C4Config.h>
#include <C4Application.h>

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
	if (const auto it = Application.SoundSystem->FindInst(name, obj))
	{
		(**it).sample.instances.erase(*it);
	}
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
	for (auto &sample : samples)
	{
		sample.instances.remove_if(
			[&](auto &inst) { return obj == inst.GetObj() && !inst.DetachObj(); });
	}
}

void C4SoundSystem::Execute()
{
	for (auto &sample : samples)
	{
		sample.Execute();
	}
}

void C4SoundSystem::LoadEffects(C4Group &group)
{
	if (!Application.AudioSystem) return;

	// Process segmented list of file types
	for (const auto fileType : { "*.wav", "*.ogg" })
	{
		char filename[_MAX_FNAME + 1];
		// Search all sound files in group
		group.ResetSearch();
		while (group.FindNextEntry(fileType, filename))
		{
			// Try to find existing sample of the same name
			const auto existingSample = std::find_if(samples.cbegin(), samples.cend(),
				[&](const auto &sample) { return SEqualNoCase(filename, sample.name.c_str()); });
			// Load sample
			StdBuf buf;
			if (!group.LoadEntry(filename, buf)) continue;
			const auto &newSample = samples.emplace_back(filename, buf.getData(), buf.getSize());
			// Overload (i.e. remove) existing sample of the same name
			if (existingSample != samples.cend()) samples.erase(existingSample);
		}
	}
}

bool C4SoundSystem::ToggleOnOff()
{
	auto &enabled = GetCfgSoundEnabled();
	return enabled = !enabled;
}

void C4SoundSystem::Sample::Execute()
{
	// Execute each instance and remove it if necessary
	instances.remove_if([](auto &inst) { return !inst.Execute(); });
}

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
	if (channel && !channel->IsPlaying()) return false;

	// Remove non-looping, inaudible sounds if half the time is up
	std::uint32_t pos = ~0;
	if (!loop && !channel && !justStarted)
	{
		pos = GetPlaybackPosition();
		if (pos > sample.duration / 2) return false;
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

	// Create/restore channel if necessary
	const auto createChannel = !channel;
	if (createChannel)
	{
		channel.emplace(sample.sample, loop);
		if (!justStarted)
		{
			if (pos == ~0) pos = GetPlaybackPosition();
			channel->SetPosition(pos);
		}
	}

	// Update volume
	channel->SetVolumeAndPan(vol, pan);
	if (createChannel) channel->Unpause();

	return true;
}

C4Object *C4SoundSystem::Instance::GetObj() const
{
	const auto ptr = std::get_if<C4Object *>(&obj);
	return ptr ? *ptr : nullptr;
}

std::uint32_t C4SoundSystem::Instance::GetPlaybackPosition() const
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - startTime).count() % sample.duration;
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

auto C4SoundSystem::FindInst(const char *wildcard, const C4Object *const obj) ->
	std::optional<decltype(Sample::instances)::iterator>
{
	const auto wildcardStr = PrepareFilename(wildcard);
	wildcard = wildcardStr.c_str();

	for (auto &sample : samples)
	{
		// Skip samples whose names do not match the wildcard
		if (!WildcardMatch(wildcard, sample.name.c_str())) continue;
		// Try to find an instance that is bound to obj
		auto it = std::find_if(sample.instances.begin(), sample.instances.end(),
			[&](const auto &inst) { return inst.GetObj() == obj; });
		if (it != sample.instances.end()) return it;
	}

	// Not found
	return {};
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

	Sample *sample;
	// Search for matching file if name contains no wildcard
	if (filenameStr.find('?') == std::string::npos)
	{
		const auto end = samples.end();
		const auto it = std::find_if(samples.begin(), end,
			[&](const auto &sample) { return SEqualNoCase(filename, sample.name.c_str()); });
		// File not found
		if (it == end) return nullptr;
		// Success: Found the file
		sample = &*it;
	}
	// Randomly select any matching file if name contains wildcard
	else
	{
		std::vector<Sample *> matches;
		for (auto &sample : samples)
		{
			if (WildcardMatch(filename, sample.name.c_str()))
			{
				matches.push_back(&sample);
			}
		}
		// File not found
		if (matches.empty()) return nullptr;
		// Success: Randomly select any of the matching files
		sample = matches[SafeRandom(matches.size())];
	}

	// Too many instances?
	if (!loop && sample->instances.size() >= MaxSoundInstances) return nullptr;

	// Already playing near?
	const auto nearIt = obj ?
		std::find_if(sample->instances.cbegin(), sample->instances.cend(),
			[&](const auto &inst) { return inst.IsNear(*obj); }) :
		std::find_if(sample->instances.cbegin(), sample->instances.cend(),
			[](const auto &inst) { return !inst.GetObj(); });
	if (nearIt != sample->instances.cend()) return nullptr;

	// Create instance
	auto &inst = sample->instances.emplace_back(*sample, loop, volume, obj, falloffDistance);
	if (!inst.Execute(true))
	{
		sample->instances.pop_back();
		return nullptr;
	}

	return &inst;
}

std::string C4SoundSystem::PrepareFilename(const char *const filename)
{
	auto result = *GetExtension(filename) ?
		filename : std::string{filename} + ".wav";
	std::replace(result.begin(), result.end(), '*', '?');
	return result;
}
