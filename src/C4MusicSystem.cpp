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

#include <C4Include.h>
#include <C4MusicSystem.h>

#include <C4Application.h>
#include <C4Random.h>
#include <C4Log.h>
#include <C4Game.h>
#include <C4Wrappers.h>

#ifdef USE_FMOD
#include <fmod_errors.h>
#endif
#ifdef HAVE_LIBSDL_MIXER
#include <SDL.h>
#endif

#include <algorithm>
#include <cstring>
#include <utility>

// helper
const char *SGetRelativePath(const char *strPath);

C4MusicSystem::C4MusicSystem()
{
	if (!Application.AudioSystem) return;

	// Load songs from global music file
	LoadDir(Config.AtExePath(C4CFN_Music));
	// Read MoreMusic.txt
	LoadMoreMusic();
}

void C4MusicSystem::Execute()
{
	if (!Application.AudioSystem) return;

	if (!Application.AudioSystem->IsMusicPlaying())
	{
		ClearPlayingSong();
		Play();
	}
}

void C4MusicSystem::Play(const char *const songname, const bool loop)
{
	if (!Application.AudioSystem) return;

	// Music disabled by user or scenario?
	if (!GetCfgMusicEnabled() || (Game.IsRunning && !Game.IsMusicEnabled)) return;

	const Song *newSong = nullptr;

	// Play specified song
	if (songname && songname[0])
	{
		newSong = FindSong(songname);
	}
	// Play random song
	else
	{
		// Create list of enabled songs excluding the most recently played song
		std::vector<Song *> enabledSongs;
		for (auto &song : songs)
		{
			if (song.enabled && &song != mostRecentlyPlayed) enabledSongs.emplace_back(&song);
		}
		// No songs? Then play most recently played song again if it is enabled
		if (enabledSongs.empty())
		{
			if (mostRecentlyPlayed && mostRecentlyPlayed->enabled) newSong = mostRecentlyPlayed;
		}
		// If there are enabled songs, randomly select one of them
		else
		{
			newSong = enabledSongs[SafeRandom(enabledSongs.size())];
		}
	}

	// File not found?
	if (!newSong) return;

	// Stop old music
	Stop();

	LogF(LoadResStr("IDS_PRC_PLAYMUSIC"), GetFilename(newSong->name.c_str()));

	// Load and play music file
	try
	{
		char *data; std::size_t size;
		if (!C4Group_ReadFile(newSong->name.c_str(), &data, &size))
		{
			throw std::runtime_error("Cannot read file");
		}
		playingFileContents.reset(data);
		playingFile.emplace(data, size);
		Application.AudioSystem->PlayMusic(*playingFile, loop);
		UpdateVolume();
		Application.AudioSystem->UnpauseMusic();
	}

	catch (const std::runtime_error &e)
	{
		LogF("Cannot play music file %s:", newSong->name.c_str());
		Log(e.what());
		ClearPlayingSong();
		return;
	}
	catch (...)
	{
		ClearPlayingSong();
		throw;
	}

	mostRecentlyPlayed = newSong;
}

void C4MusicSystem::PlayFrontendMusic()
{
	if (!Application.AudioSystem) return;

	SetPlayList("Frontend.*");
	Play();
}

void C4MusicSystem::PlayScenarioMusic(C4Group &group)
{
	if (!Application.AudioSystem) return;

	std::list<std::string> musicDirs;

	// Check if the scenario contains music
	if (std::any_of(std::cbegin(MusicFileExtensions), std::cend(MusicFileExtensions),
		[&](const auto ext) { return group.FindEntry((std::string("*.") + ext).c_str()); }))
	{
		musicDirs.emplace_back(Game.ScenarioFile.GetFullName().getData());
	}

	// Check for music folders in group set
	for (C4Group *group = nullptr; group = Game.GroupSet.FindGroup(C4GSCnt_Music, group); )
	{
		musicDirs.emplace_back(std::string() +
			group->GetFullName().getData() + DirectorySeparator + C4CFN_Music);
	}

	// Clear old songs
	if (!musicDirs.empty()) ClearSongs();

	// Load music from each directory
	for (const auto &musicDir : musicDirs)
	{
		LoadDir(musicDir.c_str());
		LogF(LoadResStr("IDS_PRC_LOCALMUSIC"), Config.AtExeRelativePath(musicDir.c_str()));
	}

	SetPlayList(nullptr);
	Play();
}

long C4MusicSystem::SetPlayList(const char *const playlist)
{
	if (!Application.AudioSystem) return 0;

	if (playlist)
	{
		// Disable all songs
		for (auto &song : songs)
		{
			song.enabled = false;
		}

		auto startIt = playlist;
		const char *const endIt = std::strchr(playlist, 0);
		while (true)
		{
			// Read next filename from playlist string
			const auto delimIt = std::find(startIt, endIt, ';');
			const std::string filename(startIt, delimIt);

			// Enable matching songs
			for (auto &song : songs)
			{
				if (song.enabled) continue;
				song.enabled = WildcardMatch(
					filename.c_str(), GetFilename(song.name.c_str()));
			}

			if (delimIt == endIt) break;
			startIt = delimIt + 1;
		}
	}
	else
	{
		/* Default: all files except the ones beginning with an at ('@')
		   and frontend and credits music. */
		for (auto &song : songs)
		{
			const auto filename = GetFilename(song.name.c_str());
			const auto ignorePrefix = { "@", "Credits.", "Frontend." };
			song.enabled = !std::any_of(ignorePrefix.begin(), ignorePrefix.end(),
				[&](const auto &prefix) { return SEqual2(filename, prefix); });
		}
	}

	// Return number of playlist entries
	return std::count_if(songs.cbegin(), songs.cend(),
		[](const auto &song) { return song.enabled; });
}

void C4MusicSystem::Stop(const int fadeoutMS)
{
	if (!Application.AudioSystem) return;

	if (fadeoutMS == 0)
	{
		ClearPlayingSong();
	}
	else
	{
		if (Application.AudioSystem->IsMusicPlaying())
		{
			Application.AudioSystem->FadeOutMusic(fadeoutMS);
		}
	}
}

bool C4MusicSystem::ToggleOnOff()
{
	auto &enabled = GetCfgMusicEnabled();
	enabled = !enabled;
	if (enabled)
	{
		Play();
	}
	else
	{
		Stop();
	}
	if (Game.IsRunning)
	{
		Game.GraphicsSystem.FlashMessageOnOff(LoadResStr("IDS_CTL_MUSIC"), enabled);
	}
	return enabled;
}

void C4MusicSystem::UpdateVolume()
{
	if (!Application.AudioSystem) return;

	if (Application.AudioSystem->IsMusicPlaying())
	{
		float volume = Config.Sound.MusicVolume / 100.0f;
		if (Game.IsRunning) volume *= Game.iMusicLevel / 100.0f;
		Application.AudioSystem->SetMusicVolume(volume);
	}
}

bool &C4MusicSystem::GetCfgMusicEnabled()
{
	return Game.IsRunning ? Config.Sound.RXMusic : Config.Sound.FEMusic;
}

void C4MusicSystem::ClearPlayingSong()
{
	Application.AudioSystem->StopMusic();
	playingFile.reset();
	playingFileContents.reset();
}

void C4MusicSystem::ClearSongs()
{
	Stop();
	mostRecentlyPlayed = nullptr;
	songs.clear();
}

auto C4MusicSystem::FindSong(const std::string &name) const -> const Song *
{
	// Try exact path
	auto pathIt = std::find_if(songs.cbegin(), songs.cend(),
		[&](const auto &song) { return name == song.name; });
	if (pathIt != songs.cend()) return &*pathIt;

	// Try exact filename
	auto filenameIt = std::find_if(songs.cbegin(), songs.cend(),
		[&](const auto &song) { return name == GetFilename(song.name.c_str()); });
	if (filenameIt != songs.cend()) return &*filenameIt;

	// Try all known extensions
	for (const auto &song : songs)
	{
		const auto songFilename = GetFilename(song.name.c_str());
		if (std::any_of(std::cbegin(MusicFileExtensions), std::cend(MusicFileExtensions),
			[&](const auto &ext) { return name + "." + ext == songFilename; }))
		{
			return &song;
		}
	}

	// Not found
	return nullptr;
}

void C4MusicSystem::LoadDir(const char *const path)
{
	// Split path
	const auto filenameStart = GetFilename(path);
	std::string file{filenameStart};
	std::string dir = (filenameStart == path ?
		std::string{} : std::string{path, filenameStart - 1});

	// Open group
	C4Group dirGroup;
	bool success = false;
	// No filename?
	if (file.empty())
	{
		// Add the whole directory
		file = "*";
	}
	// No wildcard in filename?
	else if (file.find_first_of("*?") == std::string::npos)
	{
		// Then it's either a file or a directory - do the test with C4Group
		success = dirGroup.Open(path);
		if (success)
		{
			dir = path;
			file = "*";
		}
		// If not successful, it must be a file
	}
	// Open directory group, if not already done so
	if (!success) success = dirGroup.Open(dir.c_str());
	if (!success)
	{
		LogF(LoadResStr("IDS_PRC_MUSICFILENOTFOUND"), path);
		return;
	}

	// Search music files and add them to song list
	char entry[_MAX_FNAME + 1];
	dirGroup.ResetSearch();
	while (dirGroup.FindNextEntry(file.c_str(), entry))
	{
		const auto fullPath = dir + DirectorySeparator + entry;
		const auto fileExt = GetExtension(entry);
		if (std::any_of(std::cbegin(MusicFileExtensions), std::cend(MusicFileExtensions),
			[&](const auto &musicExt) { return SEqualNoCase(fileExt, musicExt); }))
		{
			songs.emplace_back(fullPath);
		}
	}
}

void C4MusicSystem::LoadMoreMusic()
{
	// Read MoreMusic.txt file or cancel if not present
	CStdFile MoreMusicFile;
	std::uint8_t *fileContentsTmp; size_t size;
	if (!MoreMusicFile.Load(Config.AtExePath(C4CFN_MoreMusic), &fileContentsTmp, &size)) return;
	std::unique_ptr<std::uint8_t[]> fileContents(fileContentsTmp);

	// read contents
	const char *rest = reinterpret_cast<const char *>(fileContents.get());
	const auto end = rest + size;

	while (rest != end)
	{
		// Get next line
		const auto lineEnd = std::find(rest, end, '\n');
		std::string line(rest, lineEnd);
		rest = (lineEnd == end ? end : lineEnd + 1);

		// Trim
		constexpr char whitespace[] = " \t\r";
		const auto trimLeft = line.find_first_not_of(whitespace);
		// Skip if line would be empty after trimming whitespace
		if (trimLeft == std::string::npos) continue;
		const auto trimRight = line.find_last_not_of(whitespace) + 1;
		line = line.substr(trimLeft, trimRight - trimLeft);

		// Reset playlist
		if (line == "#clear")
		{
			ClearSongs();
		}
		// Comment
		else if (line[0] == '#')
		{
			continue;
		}
		// Add
		else
		{
			LoadDir(line.c_str());
		}
	}
}
