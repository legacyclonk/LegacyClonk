/*
 * LegacyClonk
 *
 * Copyright (c) 2012-2016, The OpenClonk Team and contributors
 * Copyright (c) 2024, The LegacyClonk Team and contributors
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

#include "C4Application.h"
#include "C4Awaiter.h"
#include "C4Network2UPnP.h"
#include "C4Strings.h"

#include <array>
#include <limits>

#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "miniupnpc/upnperrors.h"

struct C4Network2UPnP::Impl
{
private:
	struct PortMapping
	{
		std::array<char, 4> Protocol;
		std::array<char, C4Strings::NumberOfCharactersForDigits<std::uint16_t> + 1> InternalPort;
		std::array<char, C4Strings::NumberOfCharactersForDigits<std::uint16_t> + 1> ExternalPort;

		PortMapping(const C4Network2IOProtocol protocol, const std::uint16_t internalPort, const std::uint16_t externalPort)
		{
			if (protocol == P_TCP)
			{
				Protocol = {"TCP"};
			}
			else
			{
				Protocol = {"UDP"};
			}

			*std::to_chars(InternalPort.data(), InternalPort.data() + InternalPort.size() - 1, internalPort).ptr = '\0';
			*std::to_chars(ExternalPort.data(), ExternalPort.data() + ExternalPort.size() - 1, externalPort).ptr = '\0';
		}
	};

public:
	Impl() : logger{Application.LogSystem.CreateLogger(Config.Logging.Network2UPnP)}
	{
		task = Init();
	}

	~Impl()
	{
		if (urls.controlURL)
		{
			FreeUPNPUrls(&urls);
		}
	}

	void AddMapping(const C4Network2IOProtocol protocol, const std::uint16_t internalPort, const std::uint16_t externalPort)
	{
		task = AddMappingInt(std::move(task), protocol, internalPort, externalPort);
	}

	static C4Task::OneShot ClearMappings(std::unique_ptr<Impl> self, std::atomic_bool &done)
	{
		co_await C4Awaiter::ResumeInGlobalThreadPool();

		done.store(true, std::memory_order_release);
		done.notify_one();

		// Doing a blocking wait here to ensure the coroutine
		// is never suspended so that mapping removal succeeds
		// even if the engine is shutting down.
		std::move(self->task).Get();

		if (!self->IsInitialized())
		{
			co_return;
		}

		for (const auto &mapping : self->mappings)
		{
			const int result{UPNP_DeletePortMapping(
							self->urls.controlURL,
							self->igdData.first.servicetype,
							mapping.ExternalPort.data(),
							mapping.Protocol.data(),
							nullptr
							)};
			if (result == UPNPCOMMAND_SUCCESS)
			{
				self->logger->info("Removed port mapping {} {} -> {}:{}",
							mapping.Protocol.data(),
							mapping.ExternalPort.data(),
							self->lanAddress.data(),
							mapping.InternalPort.data()
							);
			}
			else
			{
				self->logger->error("Failed to remove port mapping {} {} -> {}:{}: {}",
							mapping.Protocol.data(),
							mapping.ExternalPort.data(),
							self->lanAddress.data(),
							mapping.InternalPort.data(),
							strupnperror(result)
							);
			}
		}
	}

	bool IsInitialized() const
	{
		return urls.controlURL;
	}

private:
	C4Task::Hot<void> Init()
	{
		co_await C4Awaiter::ResumeInGlobalThreadPool();

		logger->debug("Discovering devices");
		int error;
		const C4DeleterFunctionUniquePtr<&freeUPNPDevlist> deviceList{upnpDiscover(DiscoveryDelay, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, 0, 2, &error)};

		if (!deviceList)
		{
			logger->error("Failed to get device list: {}", ([error]
				{
					switch (error)
					{
					case UPNPDISCOVER_SUCCESS:
						return "Success";

					case UPNPDISCOVER_UNKNOWN_ERROR:
						return "Unknown error";

					case UPNPDISCOVER_SOCKET_ERROR:
						return "Socket error";

					case UPNPDISCOVER_MEMORY_ERROR:
						return "Memory error";

					default:
						return "Invalid error";
					}
				})());

			co_return;
		}

#if MINIUPNPC_API_VERSION >= 18
		std::array<char, 64> wanAddress;
		if (const int status{UPNP_GetValidIGD(deviceList.get(), &urls, &igdData, lanAddress.data(), lanAddress.size(), wanAddress.data(), wanAddress.size())}; !status)
#else
		if (const int status{UPNP_GetValidIGD(deviceList.get(), &urls, &igdData, lanAddress.data(), lanAddress.size())}; !status)
#endif

		{
			logger->error("Could not find valid IGD");
		}
	}

	C4Task::Hot<void> AddMappingInt(C4Task::Hot<void> task, const C4Network2IOProtocol protocol, const std::uint16_t internalPort, const std::uint16_t externalPort)
	{
		co_await C4Awaiter::ResumeInGlobalThreadPool();
		co_await std::move(task);

		if (!IsInitialized())
		{
			co_return;
		}

		PortMapping mapping{protocol, internalPort, externalPort > 0 ? externalPort : internalPort};

		decltype(mapping.ExternalPort) externalPortBuffer{mapping.ExternalPort};

		const std::tuple arguments{
			urls.controlURL,
			igdData.first.servicetype,
			externalPortBuffer.data(),
			mapping.InternalPort.data(),
			lanAddress.data(),
			STD_PRODUCT,
			mapping.Protocol.data(),
			nullptr,
			nullptr
		};

		const int result{externalPort == 0
					? std::apply(&UPNP_AddAnyPortMapping, std::tuple_cat(arguments, std::make_tuple(mapping.ExternalPort.data())))
					: std::apply(&UPNP_AddPortMapping, arguments)};

		if (result == UPNPCOMMAND_SUCCESS)
		{
			logger->info(
						"Added port mapping {} {} -> {}:{}",
						mapping.Protocol.data(),
						mapping.ExternalPort.data(),
						lanAddress.data(),
						mapping.InternalPort.data()
						);

			mapping.ExternalPort = std::move(externalPortBuffer);
			mappings.emplace_back(std::move(mapping));
		}
		else
		{
			logger->error(
						"Failed to add port mapping {} {} -> {}:{}: {}",
						mapping.Protocol.data(),
						mapping.ExternalPort.data(),
						lanAddress.data(),
						mapping.InternalPort.data(),
						strupnperror(result)
						);
		}
	}

	std::shared_ptr<spdlog::logger> logger;
	C4Task::Hot<void> task;
	UPNPUrls urls{};
	IGDdatas igdData;
	std::array<char, 64> lanAddress;

	std::vector<PortMapping> mappings;

	static constexpr auto DiscoveryDelay = 2000;
};

C4Network2UPnP::C4Network2UPnP()
	: impl{std::make_unique<Impl>()}
{
}

C4Network2UPnP::~C4Network2UPnP() noexcept
{
	if (impl)
	{
		std::atomic_bool done{false};
		Impl::ClearMappings(std::move(impl), done);
		done.wait(false, std::memory_order_acquire);
	}
}

void C4Network2UPnP::AddMapping(const C4Network2IOProtocol protocol, const std::uint16_t internalPort, const std::uint16_t externalPort)
{
	impl->AddMapping(protocol, internalPort, externalPort);
}
