/*
 * LegacyClonk
 *
 * Copyright (c) 2023, The LegacyClonk Team and contributors
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

#include <cassert>
#include <utility>

class C4DeletionTrackable
{
	class Tracker
	{
	public:
		constexpr Tracker(C4DeletionTrackable *const tracked) noexcept : tracked{tracked}
		{
			assert(!tracked->tracker);
			tracked->tracker = this;
		}

		Tracker(const Tracker &) = delete;

		constexpr Tracker(Tracker &&other) noexcept : tracked{std::exchange(other.tracked, nullptr)}, isDeleted{other.isDeleted}
		{
			if (!tracked)
			{
				return;
			}

			assert(tracked->tracker == &other);
			tracked->tracker = this;
		}

		Tracker &operator=(const Tracker &) = delete;

		constexpr Tracker &operator=(Tracker &&other) noexcept
		{
			Clear();
			tracked = std::exchange(other.tracked, nullptr);
			isDeleted = other.isDeleted;

			if (tracked)
			{
				assert(tracked->tracker == &other);
				tracked->tracker = this;
			}

			return *this;
		}

		constexpr ~Tracker() noexcept
		{
			Clear();
		}

		constexpr bool IsDeleted() const noexcept
		{
			return isDeleted;
		}

	private:
		constexpr void Clear() noexcept
		{
			if (tracked && !isDeleted)
			{
				assert(tracked->tracker == this);
				tracked->tracker = nullptr;
			}
		}

		C4DeletionTrackable *tracked{nullptr};
		bool isDeleted{false};

		friend C4DeletionTrackable;
	};

public:
	constexpr ~C4DeletionTrackable() noexcept
	{
		if (tracker)
		{
			tracker->isDeleted = true;
		}
	}

	[[nodiscard]] constexpr Tracker TrackDeletion() noexcept
	{
		return {this};
	}

private:
	Tracker *tracker{nullptr};
};
