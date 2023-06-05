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

#include "C4PosixSpawn.h"

#include <format>
#include <iostream>
#include <system_error>
#include <utility>

#include <spawn.h>
#include <fcntl.h>
#include <unistd.h>

extern char **environ;

namespace C4PosixSpawn
{
namespace
{
	class Fd
	{
	public:
		Fd() = delete;

		Fd(const Fd &) = delete;
		Fd& operator=(const Fd &) = delete;

		Fd(Fd &&other) noexcept : fd{std::exchange(other.fd, -1)} {}
		Fd& operator=(Fd &&other) noexcept
		{
			fd = std::exchange(other.fd, -1);
			return *this;
		}

		~Fd()
		{
			if (fd == -1)
			{
				return;
			}

			if (close(fd) != 0)
			{
				std::cerr << std::format("Warning: Fd::~Fd: close({}) failed: {}\n", fd, std::system_error{errno, std::generic_category(), "memfd_create failed"}.what());
			}
		}

		int get() const noexcept
		{
			return fd;
		}

		static Fd Open(const char *const path, const int mode)
		{
			const int fd{open(path, mode)};
			if (fd == -1)
			{
				// TODO: use {:?} when available
				throw std::system_error{errno, std::generic_category(), std::format("Fd::open: open(\"{}\", {}) failed", path, mode)};
			}

			return Fd{fd};
		}

	private:
		Fd(int fd) : fd{fd} {}

		int fd{-1};
	};

	void ThrowIfFailed(const char *const name, const int result)
	{
		if (result != 0)
		{
			throw std::system_error{result, std::generic_category(), std::string{name} + " failed"};
		}
	}

	class FileActions
	{
	public:
		FileActions()
		{
			ThrowIfFailed("posix_spawn_file_actions_init", posix_spawn_file_actions_init(&actions));
		}

		~FileActions()
		{
			if (const auto s = posix_spawn_file_actions_destroy(&actions); s != 0)
			{
				std::cerr << std::format("Warning: {}\n", std::system_error{s, std::generic_category(), "posix_spawn_file_actions_destroy failed"}.what());
			}
		}

		FileActions(const FileActions &) = delete;
		FileActions(FileActions &&) = delete;

		FileActions &operator=(const FileActions &) = delete;
		FileActions &operator=(FileActions &&) = delete;

	public:
		void AddClose(const int fd)
		{
			ThrowIfFailed("posix_spawn_file_actions_addclose", posix_spawn_file_actions_addclose(&actions, fd));
		}

		void AddDup2(const int fd, const int newFd)
		{
			ThrowIfFailed("posix_spawn_file_actions_adddup2", posix_spawn_file_actions_adddup2(&actions, fd, newFd));
		}

		void AddOpen(const int fd, const char *const path, const int oflag, const mode_t mode)
		{
			ThrowIfFailed("posix_spawn_file_actions_addopen", posix_spawn_file_actions_addopen(&actions, fd, path, oflag, mode));
		}

		operator posix_spawn_file_actions_t*()
		{
			return &actions;
		}

	private:
		posix_spawn_file_actions_t actions;
	};
}

pid_t SpawnP(const std::vector<std::string> &arguments)
{
	std::vector<const char *> argv;
	argv.reserve(arguments.size() + 1);

	for (const auto &arg : arguments)
	{
		argv.push_back(arg.c_str());
	}

	argv.push_back(nullptr);

	FileActions fileActions;

	const auto devNull = Fd::Open("/dev/null", O_RDWR);

	fileActions.AddDup2(devNull.get(), STDIN_FILENO);
	fileActions.AddDup2(devNull.get(), STDOUT_FILENO);
	fileActions.AddDup2(devNull.get(), STDERR_FILENO);

	pid_t pid;
	ThrowIfFailed("posix_spawnp", posix_spawnp(&pid, argv[0], fileActions, nullptr, const_cast<char **>(argv.data()), environ));
	return pid;
}

}
