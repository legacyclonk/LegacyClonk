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

C4AudioSystem::C4AudioSystem() {}
void C4AudioSystem::FadeOutMusic(const std::int32_t ms) {}
bool C4AudioSystem::IsMusicPlaying() const { return false; }
void C4AudioSystem::PlayMusic(const MusicFile &music, const bool loop) {}
void C4AudioSystem::SetMusicVolume(const std::int32_t volume) {}
void C4AudioSystem::StopMusic() {}
void C4AudioSystem::UnpauseMusic() {}

C4AudioSystem::MusicFile::MusicFile(const void *const buf, const std::size_t size) {}

C4AudioSystem::SoundChannel::SoundChannel(const SoundFile &sound, const bool loop) {}
bool C4AudioSystem::SoundChannel::IsPlaying() const { return false; }
void C4AudioSystem::SoundChannel::SetPosition(const std::uint32_t ms) {}
void C4AudioSystem::SoundChannel::SetVolumeAndPan(const std::int32_t volume, const std::int32_t pan) {}
void C4AudioSystem::SoundChannel::Unpause() {}

C4AudioSystem::SoundFile::SoundFile(const void *const buf, const std::size_t size) {}
std::uint32_t C4AudioSystem::SoundFile::GetDuration() const { return 0; }
