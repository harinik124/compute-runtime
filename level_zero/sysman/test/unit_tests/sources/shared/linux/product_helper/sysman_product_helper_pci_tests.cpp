/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/sysman/source/shared/linux/product_helper/sysman_product_helper.h"
#include "level_zero/sysman/test/unit_tests/sources/linux/mock_sysman_fixture.h"

namespace L0 {
namespace Sysman {
namespace ult {

const std::string telem1NodePath("/sys/class/intel_pmt/telem1");
const std::string telem2NodePath("/sys/class/intel_pmt/telem2");
const std::string telem3NodePath("/sys/class/intel_pmt/telem3");
const std::string telem4NodePath("/sys/class/intel_pmt/telem4");

const std::string mockValidGuid("0x5e2F8210");
const std::string telem3OffsetString = "10";
const std::string telem3GuidFile = "/sys/class/intel_pmt/telem3/guid";
const std::string telem3OffsetFile = "/sys/class/intel_pmt/telem3/offset";
const std::string telem3TelemFile = "/sys/class/intel_pmt/telem3/telem";

const std::unordered_map<std::string, int> telem3FileAndFdMap = {{telem3GuidFile, 1},
                                                                 {telem3OffsetFile, 2},
                                                                 {telem3TelemFile, 3}};

constexpr uint64_t telem3OffsetValue = 10;
constexpr uint64_t mockRxCounterLsbOffset = telem3OffsetValue + 70;
constexpr uint64_t mockRxCounterMsbOffset = telem3OffsetValue + 69;
constexpr uint64_t mockTxCounterLsbOffset = telem3OffsetValue + 72;
constexpr uint64_t mockTxCounterMsbOffset = telem3OffsetValue + 71;
constexpr uint64_t mockRxPacketCounterLsbOffset = telem3OffsetValue + 74;
constexpr uint64_t mockRxPacketCounterMsbOffset = telem3OffsetValue + 73;
constexpr uint64_t mockTxPacketCounterLsbOffset = telem3OffsetValue + 76;
constexpr uint64_t mockTxPacketCounterMsbOffset = telem3OffsetValue + 75;

constexpr uint32_t mockRxCounterLsb = 0xA2u;
constexpr uint32_t mockRxCounterMsb = 0xF5u;
constexpr uint32_t mockTxCounterLsb = 0xCDu;
constexpr uint32_t mockTxCounterMsb = 0xABu;
constexpr uint32_t mockRxPacketCounterLsb = 0xA0u;
constexpr uint32_t mockRxPacketCounterMsb = 0xBCu;
constexpr uint32_t mockTxPacketCounterLsb = 0xFAu;
constexpr uint32_t mockTxPacketCounterMsb = 0xFFu;

using SysmanProductHelperPciTest = SysmanDeviceFixture;
using IsNotBMG = IsNotWithinProducts<IGFX_BMG, IGFX_BMG>;

static int mockReadLinkSuccess(const char *path, char *buf, size_t bufsize) {
    std::map<std::string, std::string> fileNameLinkMap = {
        {telem1NodePath, "../../devices/pci0000:00/0000:00:0a.0/intel-vsec.telemetry.0/intel_pmt/telem1/"},
        {telem2NodePath, "../../devices/pci0000:00/0000:00:0a.0/intel-vsec.telemetry.0/intel_pmt/telem2/"},
        {telem3NodePath, "../../devices/pci0000:00/0000:00:01.0/0000:01:00.0/0000:02:01.0/0000:03:00.0/intel-vsec.telemetry.1/intel_pmt/telem3/"},
        {telem4NodePath, "../../devices/pci0000:00/0000:00:01.0/0000:01:00.0/0000:02:01.0/0000:03:00.0/intel-vsec.telemetry.1/intel_pmt/telem4/"},
    };
    auto it = fileNameLinkMap.find(std::string(path));
    if (it != fileNameLinkMap.end()) {
        std::memcpy(buf, it->second.c_str(), it->second.size());
        return static_cast<int>(it->second.size());
    }
    return -1;
}

static int mockOpenSuccess(const char *pathname, int flags) {
    std::string strPathName(pathname);
    if (telem3FileAndFdMap.find(strPathName) != telem3FileAndFdMap.end()) {
        return telem3FileAndFdMap.at(strPathName);
    }
    return -1;
}

static ssize_t mockPreadSuccess(int fd, void *buf, size_t count, off_t offset) {
    if (fd == telem3FileAndFdMap.at(telem3GuidFile)) {
        memcpy(buf, mockValidGuid.data(), count);
    } else if (fd == telem3FileAndFdMap.at(telem3OffsetFile)) {
        memcpy(buf, telem3OffsetString.data(), count);
    } else if (fd == telem3FileAndFdMap.at(telem3TelemFile)) {
        switch (offset) {
        case mockRxCounterLsbOffset:
            memcpy(buf, &mockRxCounterLsb, count);
            break;
        case mockRxCounterMsbOffset:
            memcpy(buf, &mockRxCounterMsb, count);
            break;
        case mockTxCounterLsbOffset:
            memcpy(buf, &mockTxCounterLsb, count);
            break;
        case mockTxCounterMsbOffset:
            memcpy(buf, &mockTxCounterMsb, count);
            break;
        case mockRxPacketCounterLsbOffset:
            memcpy(buf, &mockRxPacketCounterLsb, count);
            break;
        case mockRxPacketCounterMsbOffset:
            memcpy(buf, &mockRxPacketCounterMsb, count);
            break;
        case mockTxPacketCounterLsbOffset:
            memcpy(buf, &mockTxPacketCounterLsb, count);
            break;
        case mockTxPacketCounterMsbOffset:
            memcpy(buf, &mockTxPacketCounterMsb, count);
            break;
        }
    }
    return count;
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledAndTelemNodesAreNotAvailableThenCallFails, IsBMG) {
    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);
    zes_pci_stats_t stats;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledAndReadGuidFromPmtUtilFailsThenCallFails, IsBMG) {
    VariableBackup<decltype(NEO::SysCalls::sysCallsReadlink)> mockReadLink(&NEO::SysCalls::sysCallsReadlink, &mockReadLinkSuccess);

    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);
    zes_pci_stats_t stats;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledAndGuidIsNotPresentInGuidToKeyOffsetMapThenCallFails, IsBMG) {
    static std::string dummyGuid("dummyGuid");
    VariableBackup<decltype(NEO::SysCalls::sysCallsReadlink)> mockReadLink(&NEO::SysCalls::sysCallsReadlink, &mockReadLinkSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsOpen)> mockOpen(&NEO::SysCalls::sysCallsOpen, &mockOpenSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsPread)> mockPread(&NEO::SysCalls::sysCallsPread, [](int fd, void *buf, size_t count, off_t offset) -> ssize_t {
        if (fd == telem3FileAndFdMap.at(telem3GuidFile)) {
            memcpy(buf, dummyGuid.data(), count);
        }
        return count;
    });
    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);
    zes_pci_stats_t stats;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledAndReadOffsetFromPmtUtilFailsDueToPreadSysCallThenCallFails, IsBMG) {
    VariableBackup<decltype(NEO::SysCalls::sysCallsReadlink)> mockReadLink(&NEO::SysCalls::sysCallsReadlink, &mockReadLinkSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsOpen)> mockOpen(&NEO::SysCalls::sysCallsOpen, &mockOpenSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsPread)> mockPread(&NEO::SysCalls::sysCallsPread, [](int fd, void *buf, size_t count, off_t offset) -> ssize_t {
        if (fd == telem3FileAndFdMap.at(telem3GuidFile)) {
            memcpy(buf, mockValidGuid.data(), count);
        } else if (fd == telem3FileAndFdMap.at(telem3OffsetFile)) {
            count = -1;
        }
        return count;
    });
    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);
    zes_pci_stats_t stats;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledAndTelemFileIsNotAvailableThenCallFails, IsBMG) {
    VariableBackup<decltype(NEO::SysCalls::sysCallsReadlink)> mockReadLink(&NEO::SysCalls::sysCallsReadlink, &mockReadLinkSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsPread)> mockPread(&NEO::SysCalls::sysCallsPread, &mockPreadSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsOpen)> mockOpen(&NEO::SysCalls::sysCallsOpen, [](const char *pathname, int flags) -> int {
        std::string strPathName(pathname);
        if (strPathName == telem3TelemFile) {
            return -1;
        }
        return (telem3FileAndFdMap.find(strPathName) != telem3FileAndFdMap.end()) ? telem3FileAndFdMap.at(strPathName) : -1;
    });

    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);
    zes_pci_stats_t stats;
    EXPECT_EQ(ZE_RESULT_ERROR_NOT_AVAILABLE, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledAndReadValueFromPmtUtilFailsThenCallFails, IsBMG) {
    static int readFailCount = 1;
    VariableBackup<decltype(NEO::SysCalls::sysCallsReadlink)> mockReadLink(&NEO::SysCalls::sysCallsReadlink, &mockReadLinkSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsOpen)> mockOpen(&NEO::SysCalls::sysCallsOpen, &mockOpenSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsPread)> mockPread(&NEO::SysCalls::sysCallsPread, [](int fd, void *buf, size_t count, off_t offset) -> ssize_t {
        if (fd == telem3FileAndFdMap.at(telem3GuidFile)) {
            memcpy(buf, mockValidGuid.data(), count);
        } else if (fd == telem3FileAndFdMap.at(telem3OffsetFile)) {
            memcpy(buf, telem3OffsetString.data(), count);
        } else if (fd == telem3FileAndFdMap.at(telem3TelemFile)) {
            switch (offset) {
            case mockRxCounterLsbOffset:
                count = (readFailCount == 1) ? -1 : sizeof(uint32_t);
                break;
            case mockRxCounterMsbOffset:
                count = (readFailCount == 2) ? -1 : sizeof(uint32_t);
                break;
            case mockTxCounterLsbOffset:
                count = (readFailCount == 3) ? -1 : sizeof(uint32_t);
                break;
            case mockTxCounterMsbOffset:
                count = (readFailCount == 4) ? -1 : sizeof(uint32_t);
                break;
            case mockRxPacketCounterLsbOffset:
                count = (readFailCount == 5) ? -1 : sizeof(uint32_t);
                break;
            case mockRxPacketCounterMsbOffset:
                count = (readFailCount == 6) ? -1 : sizeof(uint32_t);
                break;
            case mockTxPacketCounterLsbOffset:
                count = (readFailCount == 7) ? -1 : sizeof(uint32_t);
                break;
            case mockTxPacketCounterMsbOffset:
                count = (readFailCount == 8) ? -1 : sizeof(uint32_t);
                break;
            }
        }
        return count;
    });

    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);
    zes_pci_stats_t stats;
    while (readFailCount <= 8) {
        EXPECT_EQ(ZE_RESULT_ERROR_NOT_AVAILABLE, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));
        readFailCount++;
    }
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledThenCallSucceeds, IsBMG) {
    VariableBackup<decltype(NEO::SysCalls::sysCallsReadlink)> mockReadLink(&NEO::SysCalls::sysCallsReadlink, &mockReadLinkSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsOpen)> mockOpen(&NEO::SysCalls::sysCallsOpen, &mockOpenSuccess);
    VariableBackup<decltype(NEO::SysCalls::sysCallsPread)> mockPread(&NEO::SysCalls::sysCallsPread, &mockPreadSuccess);

    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);

    zes_pci_stats_t stats;
    EXPECT_EQ(ZE_RESULT_SUCCESS, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));

    uint64_t mockRxCounter = PACK_INTO_64BIT(mockRxCounterMsb, mockRxCounterLsb);
    EXPECT_EQ(mockRxCounter, stats.rxCounter);

    uint64_t mockTxCounter = PACK_INTO_64BIT(mockTxCounterMsb, mockTxCounterLsb);
    EXPECT_EQ(mockTxCounter, stats.txCounter);

    uint64_t mockRxPacketCounter = PACK_INTO_64BIT(mockRxPacketCounterMsb, mockRxPacketCounterLsb);
    uint64_t mockTxPacketCounter = PACK_INTO_64BIT(mockTxPacketCounterMsb, mockTxPacketCounterLsb);
    EXPECT_EQ(mockRxPacketCounter + mockTxPacketCounter, stats.packetCounter);
}

HWTEST2_F(SysmanProductHelperPciTest, GivenSysmanProductHelperInstanceWhenGetPciStatsIsCalledThenCallFails, IsNotBMG) {
    auto pSysmanProductHelper = L0::Sysman::SysmanProductHelper::create(defaultHwInfo->platform.eProductFamily);
    zes_pci_stats_t stats;
    EXPECT_EQ(ZE_RESULT_ERROR_UNSUPPORTED_FEATURE, pSysmanProductHelper->getPciStats(&stats, pLinuxSysmanImp));
}

} // namespace ult
} // namespace Sysman
} // namespace L0