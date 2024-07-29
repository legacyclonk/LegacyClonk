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

// Handles the sound bank and plays effects.

#pragma once

#include <C4AudioSystem.h>
#include <C4Object.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <variant>

class C4Section;

class C4SoundSystem
{
public:
	struct Position
	{
		C4Section *const Section;
		const int32_t X;
		const int32_t Y;

		constexpr bool operator==(const Position &other) const = default;
	};

	struct GlobalSoundMarker
	{
		constexpr bool operator==(GlobalSoundMarker) const noexcept { return true; }
	};

	static inline constexpr GlobalSoundMarker GlobalSound{};

	using TargetVariant = std::variant<C4Object *, Position, C4Section *, GlobalSoundMarker>;

public:
	static constexpr std::int32_t AudibilityRadius = 700;
	static constexpr std::int32_t NearSoundRadius = 50;

	C4SoundSystem();
	C4SoundSystem(const C4SoundSystem &) = delete;
	C4SoundSystem(C4SoundSystem &&) = delete;
	~C4SoundSystem() = default;
	C4SoundSystem &operator=(const C4SoundSystem &) = delete;
	C4SoundSystem &operator=(C4SoundSystem &&) = delete;

	// Detaches the specified object from all sound instances.
	void ClearPointers(const C4Object *obj);
	void ClearSectionPointers(C4Section &section);
	void Execute();
	// Load sounds from the specified folder
	void LoadEffects(C4Group &group);
	// Call whenever the user wants to toggle sound playback
	bool ToggleOnOff();

private:
	struct Instance;

	struct Sample
	{
		const std::string name;
		const std::unique_ptr<C4AudioSystem::SoundFile> sample;
		const std::uint32_t duration;
		std::list<Instance> instances;

		Sample(const char *const name, const void *const buf, const std::size_t size);
		Sample(const Sample &) = delete;
		Sample(Sample &&) = delete;
		~Sample() = default;
		Sample &operator=(const Sample &) = delete;
		Sample &operator=(Sample &&) = delete;

		void Execute();
	};

	struct Instance
	{
		Instance(Sample &sample, bool loop, std::int32_t volume,
			TargetVariant target, std::int32_t falloffDistance)
			: sample{sample}, loop{loop}, volume{volume},
			target{target}, falloffDistance{falloffDistance},
			startTime{std::chrono::steady_clock::now()} {}
		Instance(const Instance &) = delete;
		Instance(Instance &&) = delete;
		~Instance() = default;
		Instance &operator=(const Instance &) = delete;
		Instance &operator=(Instance &&) = delete;

		Sample &sample;
		std::unique_ptr<C4AudioSystem::SoundChannel> channel;
		const bool loop;
		std::int32_t volume, pan{0};
		TargetVariant target;
		const std::int32_t falloffDistance;
		const std::chrono::time_point<std::chrono::steady_clock> startTime;

		// Returns false if this instance should be removed.
		bool DetachObj();
		// Returns false if this instance should be removed.
		bool Execute(bool justStarted = false);
		C4Object *GetObj() const;
		// Estimates playback position (milliseconds)
		std::uint32_t GetPlaybackPosition() const;
		bool IsNear(const C4Object &obj) const;
	};

	static constexpr std::int32_t MaxSoundInstances = 20;
	std::list<Sample> samples;

	// Returns a sound instance that matches the specified name and object.
	std::optional<decltype(Sample::instances)::iterator> FindInst(
		const char *wildcard, TargetVariant target);
	// Returns a reference to the "sound enabled" config entry of the current game mode
	static bool &GetCfgSoundEnabled();
	static void GetVolumeByPos(C4Section &section, std::int32_t x, std::int32_t y,
							   std::int32_t &volume, std::int32_t &pan);
	Instance *NewInstance(const char *filename, bool loop,
		std::int32_t volume, std::int32_t pan, TargetVariant target, std::int32_t falloffDistance);
	// Adds default file extension if missing and replaces "*" with "?"
	static std::string PrepareFilename(const char *filename);

	friend bool IsSoundPlaying(const char *, TargetVariant);
	friend void SoundLevel(const char *, TargetVariant, std::int32_t);
	friend bool StartSoundEffect(const char *, bool, std::int32_t, C4SoundSystem::TargetVariant, std::int32_t);
	friend void StartSoundEffectAt(const char *, C4Section &, std::int32_t, std::int32_t);
	friend void StopSoundEffect(const char *, TargetVariant);
};

bool IsSoundPlaying(const char *name, C4SoundSystem::TargetVariant target);
void SoundLevel(const char *name, C4SoundSystem::TargetVariant target, std::int32_t iLevel);
bool StartSoundEffect(const char *name, bool loop = false, std::int32_t volume = 100,
	C4SoundSystem::TargetVariant target = C4SoundSystem::GlobalSound, std::int32_t falloffDistance = 0);
void StartSoundEffectAt(const char *name, C4Section &section, std::int32_t x, std::int32_t y);
void StopSoundEffect(const char *name, C4SoundSystem::TargetVariant target);
