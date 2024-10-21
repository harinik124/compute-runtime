/*
 * Copyright (C) 2020-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include <chrono>
#include <string_view>


namespace L0 {

inline constexpr std::string_view vendorIntel("Intel(R) Corporation");
inline constexpr std::string_view unknown("unknown");
inline constexpr std::string_view intelPciId("0x8086");
inline constexpr std::string_view guid64BitMemoryCounters("0xb15a0ede");
inline constexpr uint32_t mbpsToBytesPerSecond = 125000;
inline constexpr double milliVoltsFactor = 1000.0;
inline constexpr uint32_t maxRasErrorCategoryCount = 7;
inline constexpr uint32_t maxRasErrorCategoryExpCount = 10;

struct SteadyClock {
    typedef std::chrono::duration<uint64_t, std::milli> duration;
    typedef duration::rep rep;
    typedef duration::period period;
    typedef std::chrono::time_point<SteadyClock> time_point;
    static time_point now() noexcept {
        static auto epoch = std::chrono::steady_clock::now();
        return time_point(std::chrono::duration_cast<duration>(std::chrono::steady_clock::now() - epoch));
    }
};

namespace PciLinkSpeeds {
inline constexpr double pci2Dot5GigaTransfersPerSecond = 2.5;
inline constexpr double pci5GigaTransfersPerSecond = 5.0;
inline constexpr double pci8GigaTransfersPerSecond = 8.0;
inline constexpr double pci16GigaTransfersPerSecond = 16.0;
inline constexpr double pci32GigaTransfersPerSecond = 32.0;

} // namespace PciLinkSpeeds
enum class PciGenerations {
    PciGen1 = 1,
    PciGen2,
    PciGen3,
    PciGen4,
    PciGen5,
};

inline constexpr uint8_t maxPciBars = 6;
// Linux kernel would report 255 link width, as an indication of unknown.
inline constexpr uint32_t unknownPcieLinkWidth = 255u;

inline constexpr uint32_t microSecondsToNanoSeconds = 1000u;

inline constexpr uint64_t convertJouleToMicroJoule = 1000000u;
inline constexpr uint64_t minTimeoutModeHeartbeat = 5000u;
inline constexpr uint64_t minTimeoutInMicroSeconds = 1000u;
inline constexpr uint16_t milliSecsToMicroSecs = 1000;
inline constexpr uint32_t milliFactor = 1000u;
inline constexpr uint32_t microFacor = milliFactor * milliFactor;
inline constexpr uint64_t gigaUnitTransferToUnitTransfer = 1000 * 1000 * 1000;

inline constexpr int32_t memoryBusWidth = 128; // bus width in bytes
inline constexpr int32_t numMemoryChannels = 8;
inline constexpr uint32_t unknownMemoryType = UINT32_MAX;

inline constexpr uint32_t bits(uint32_t x, uint32_t at, uint32_t width) {
    return (((x) >> (at)) & ((1 << (width)) - 1));
}
} // namespace L0