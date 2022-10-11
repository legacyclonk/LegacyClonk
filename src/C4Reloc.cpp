/*
 * LegacyClonk
 *
 * Copyright (c) 2021, The LegacyClonk Team and contributors
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

#include "C4Components.h"
#include "C4Config.h"
#include "C4Group.h"
#include "C4Reloc.h"

#include <array>

C4Reloc Reloc;

C4Reloc::ConstIterator &C4Reloc::ConstIterator::operator++()
{
	if (subDirectoryIterator == fs::recursive_directory_iterator{})
	{
		if (pathListIterator->PathType == C4Reloc::PathType::IncludingSubdirectories)
		{
			try
			{
				subDirectoryIterator = fs::recursive_directory_iterator{pathListIterator->Path};
				return *this;
			}
			catch (const fs::filesystem_error &)
			{
			}
		}
	}
	else
	{
		while (++subDirectoryIterator != fs::recursive_directory_iterator{})
		{
			if (subDirectoryIterator->is_directory())
			{
				return *this;
			}
		}
	}

	++pathListIterator;
	return *this;
}

auto C4Reloc::ConstIterator::operator*() const -> const_reference
{
	if (subDirectoryIterator != fs::recursive_directory_iterator{})
	{
		temporaryPathInfo = {*subDirectoryIterator, PathType::Regular};
		return temporaryPathInfo;
	}

	return *pathListIterator;
}

bool operator==(const C4Reloc::ConstIterator &left, const C4Reloc::ConstIterator &right)
{
	return
			left.subDirectoryIterator == right.subDirectoryIterator
			&& left.pathListIterator == right.pathListIterator;
}

bool operator!=(const C4Reloc::ConstIterator &left, const C4Reloc::ConstIterator &right)
{
	return !(left == right);
}

void C4Reloc::Init()
{
	paths.clear();

	// The system folder (i.e. installation path) has higher priority than the user path
	// Although this is counter-intuitive (the user may want to overload system files in the user path),
	// people had trouble when they downloaded e.g. an Objects.c4d file in a network lobby and that copy permanently
	// ruined their LegacyClonk installation with no obvious way to fix it.
	// Not even reinstalling would fix the problem because reinstallation does not overwrite user data.
	// We currently don't have any valid case where overloading system files would make sense so just give higher priority to the system path for now.

#ifndef __APPLE__
	// Add planet subfolder with highest priority because it's used when starting directly from the repository with binaries in the root folder
	const auto planet = fs::path{Config.General.ExePath} / "planet";
	std::error_code ec;
	if (DirectoryExists(planet))
	{
		// Only add planet if it's a valid contents folder.
		// Because users may create a folder "planet" in their source repos.
		if (ItemExists(planet / C4CFN_System))
		{
			AddPath(planet);
		}
	}
	else
#endif
		// Add main system path (unless it's using planet/ anyway, in which we would just slow down scenario enumeration by looking through the whole source folder)
		AddPath(Config.General.SystemDataPath);

	// Add user path for additional data (player files, user scenarios, etc.)
	AddPath(Config.General.UserDataPath, PathType::PreferredInstallationLocation);
}

bool C4Reloc::AddPath(fs::path path, PathType pathType)
{
	std::array<char, _MAX_PATH> buffer;
	const std::string pathAsString{path.string()};
	std::fill(std::copy(pathAsString.cbegin(), pathAsString.cend(), buffer.begin()), buffer.end(), '\0');
	Config.ExpandEnvironmentVariables(buffer.data(), _MAX_PATH - 1);
	path = fs::path{buffer.data()};

	if (!path.is_absolute())
	{
		return false;
	}

	try
	{
		path = fs::canonical(path);
	}
	catch (const fs::filesystem_error &)
	{
		return false;
	}

	if (std::find_if(std::cbegin(paths), std::cend(paths), [&path](const auto &pathInfo)
	{
		return pathInfo.Path == path;
	}) != std::cend(paths))
	{
		return false;
	}

	paths.push_back({path, pathType});
	return true;
}

bool C4Reloc::Open(C4Group &group, const fs::path &path) const
{
	if (path.is_absolute())
	{
		return group.Open(path.string().c_str());
	}

	for (const auto &pathInfo : *this)
	{
		try
		{
			const auto p = fs::weakly_canonical(pathInfo.Path / path);
			if (group.Open(p.string().c_str()))
			{
				return true;
			}
		}
		catch (const fs::filesystem_error &)
		{
			return false;
		}
	}

	return false;
}

std::optional<fs::path> C4Reloc::LocateItem(const fs::path &path) const
{
	if (path.is_absolute())
	{
		return {path};
	}

	for (const auto &pathInfo : *this)
	{
		std::error_code ec;
		const fs::path subPath{fs::canonical(pathInfo.Path / path, ec)};

		if (!ec)
		{
			return {subPath};
		}
		else if (ec == std::errc::no_such_file_or_directory)
		{
			continue;
		}
		else
		{
			break;
		}
	}

	return {};
}

std::vector<fs::path> C4Reloc::LocateItems(const fs::path &path) const
{
	if (path.is_absolute())
	{
		return {path};
	}

	std::vector<fs::path> result;

	for (const auto &pathInfo : *this)
	{
		std::error_code ec;
		const fs::path subPath{fs::canonical(pathInfo.Path / path, ec)};

		if (!ec)
		{
			result.emplace_back(subPath);
		}
		else if (ec == std::errc::no_such_file_or_directory)
		{
			continue;
		}
		else
		{
			break;
		}
	}

	return result;
}

fs::path C4Reloc::GetRelative(const fs::path &path) const
{
	if (path.is_absolute())
	{
		return path;
	}

	for (const auto &pathInfo : *this)
	{
		std::error_code ec;
		if (const fs::path relativePath{fs::relative(path, pathInfo.Path, ec)}; relativePath != fs::path{})
		{
			return relativePath;
		}
	}

	return path;
}

C4Reloc::iterator C4Reloc::begin() const
{
	return {paths.cbegin()};
}

C4Reloc::iterator C4Reloc::end() const
{
	return {paths.cend()};
}
