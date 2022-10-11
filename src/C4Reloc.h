/*
 * LegacyClonk
 *
 * Copyright (c) 2011-2016, The OpenClonk Team and contributors
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

#pragma once

#include "StdFile.h"

#include <stack>
#include <string>
#include <string_view>
#include <optional>
#include <vector>

class C4Group;

class C4Reloc
{
public:
	enum class PathType
	{
		Regular,
		PreferredInstallationLocation,
		IncludingSubdirectories
	};

	struct PathInfo
	{
		fs::path Path;
		PathType PathType;
	};

	using PathList = std::vector<PathInfo>;

	class ConstIterator
	{
	public:
		using value_type = PathInfo;
		using difference_type = std::ptrdiff_t;
		using pointer = const value_type *;
		using reference = value_type &;
		using const_reference = const value_type &;
		using iterator_category = std::forward_iterator_tag;

	public:
		ConstIterator(PathList::const_iterator pathListIterator) : pathListIterator{pathListIterator} {}
		ConstIterator(const ConstIterator &) = default;
		ConstIterator(ConstIterator &&) = default;
		ConstIterator &operator=(const ConstIterator &other) = default;
		ConstIterator &operator=(ConstIterator &&other) = default;

	public:
		ConstIterator &operator++();
		const_reference operator*() const;
		pointer operator->() const { return &(**this); }

		friend bool operator==(const ConstIterator &left, const ConstIterator &right);
		friend bool operator!=(const ConstIterator &left, const ConstIterator &right);

	private:
		mutable PathInfo temporaryPathInfo;
		PathList::const_iterator pathListIterator;
		fs::recursive_directory_iterator subDirectoryIterator;
	};

	using const_iterator = ConstIterator;
	using iterator = const_iterator;

	// Can also be used for re-init, drops custom paths added with AddPath.
	// Make sure to call after Config.Load.
	void Init();

	bool AddPath(fs::path path, PathType pathType = PathType::Regular);
	bool Open(C4Group &group, const fs::path &path) const;
	std::optional<fs::path> LocateItem(const fs::path &path) const;
	std::vector<fs::path> LocateItems(const fs::path &path) const;
	fs::path GetRelative(const fs::path &path) const;

	iterator begin() const;
	iterator end() const;
	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }

private:
	PathList paths;
};

extern C4Reloc Reloc;
