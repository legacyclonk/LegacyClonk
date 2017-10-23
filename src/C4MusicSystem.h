/*
 * LegacyClonk
 *
 * Copyright (c) 1998-2000, Matthes Bender (RedWolf Design)
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

// Handles song list and music playback.

#pragma once

#include <C4AudioSystem.h>
#include <C4Group.h>

#include <cstdint>
#include <initializer_list>
#include <list>
#include <memory>
#include <optional>
#include <string>

class C4MusicSystem
{
public:
	C4MusicSystem();
	C4MusicSystem(const C4MusicSystem &) = delete;
	C4MusicSystem(C4MusicSystem &&) = delete;
	C4MusicSystem &operator=(const C4MusicSystem &) = delete;
	~C4MusicSystem() { Stop(); }

	void Execute();
	/* Does nothing if user did not enable music for current mode (frontend/game).
	   Otherwise start playing. If already playing, stop and restart. */
	void Play(const char *songname = nullptr, bool loop = false);
	void PlayFrontendMusic();
	void PlayScenarioMusic(C4Group &);
	long SetPlayList(const char *playlist);
	void Stop(int fadeoutMS = 0);
	bool ToggleOnOff();
	void UpdateVolume();

private:
	struct Song
	{
		const std::string name;
		bool enabled = false;

		Song() = delete;
		Song(const std::string &name) : name{name} {}
		Song(const Song &) = delete;
		Song(Song &&) = delete;
		Song &operator=(const Song &) = delete;
		~Song() = default;
	};

	static constexpr std::initializer_list<const char *> MusicFileExtensions{ "it", "mid", "mod", "mp3", "ogg", "s3m", "xm" };

	std::list<Song> songs;
	const Song *mostRecentlyPlayed{};

	// Valid when a song is currently playing
	std::unique_ptr<const char[]> playingFileContents;
	std::optional<C4AudioSystem::MusicFile> playingFile;

	// Returns a reference to the "music enabled" config entry of the current game mode
	static bool &GetCfgMusicEnabled();

	void ClearPlayingSong();
	void ClearSongs();
	const Song *FindSong(const std::string &name) const;
	void LoadDir(const char *path);
	void LoadMoreMusic();
};
