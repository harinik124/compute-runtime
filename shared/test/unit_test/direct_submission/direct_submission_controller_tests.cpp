/*
 * Copyright (C) 2019-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/os_interface/os_context.h"
#include "shared/source/os_interface/os_thread.h"
#include "shared/test/common/helpers/debug_manager_state_restore.h"
#include "shared/test/common/helpers/engine_descriptor_helper.h"
#include "shared/test/common/mocks/mock_command_stream_receiver.h"
#include "shared/test/common/mocks/mock_execution_environment.h"
#include "shared/test/common/test_macros/test.h"
#include "shared/test/unit_test/direct_submission/direct_submission_controller_mock.h"

namespace NEO {

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerTimeoutWhenCreateObjectThenTimeoutIsEqualWithDebugFlag) {
    DebugManagerStateRestore restorer;
    debugManager.flags.DirectSubmissionControllerTimeout.set(14);

    DirectSubmissionControllerMock controller;

    EXPECT_EQ(controller.timeout.count(), 14);
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllertimeoutDivisorWhenCreateObjectThentimeoutDivisorIsEqualWithDebugFlag) {
    DebugManagerStateRestore restorer;
    debugManager.flags.DirectSubmissionControllerDivisor.set(4);

    DirectSubmissionControllerMock controller;

    EXPECT_EQ(controller.timeoutDivisor, 4);
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerWhenRegisterDirectSubmissionWorksThenItIsMonitoringItsState) {
    MockExecutionEnvironment executionEnvironment;
    executionEnvironment.prepareRootDeviceEnvironments(1);
    executionEnvironment.initializeMemoryManager();

    DeviceBitfield deviceBitfield(1);
    MockCommandStreamReceiver csr(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext(OsContext::create(nullptr, 0, 0,
                                                           EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                        PreemptionMode::ThreadGroup, deviceBitfield)));
    csr.setupContext(*osContext.get());
    csr.taskCount.store(5u);

    DirectSubmissionControllerMock controller;
    controller.registerDirectSubmission(&csr);

    controller.checkNewSubmissions();
    EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
    EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 5u);

    csr.taskCount.store(6u);
    controller.checkNewSubmissions();
    EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
    EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 6u);

    controller.checkNewSubmissions();
    EXPECT_TRUE(controller.directSubmissions[&csr].isStopped);
    EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 6u);

    controller.checkNewSubmissions();
    EXPECT_TRUE(controller.directSubmissions[&csr].isStopped);
    EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 6u);

    csr.taskCount.store(8u);
    controller.checkNewSubmissions();
    EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
    EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 8u);

    controller.unregisterDirectSubmission(&csr);
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerAndDivisorDisabledWhenIncreaseTimeoutEnabledThenTimeoutIsIncreased) {
    DebugManagerStateRestore restorer;
    debugManager.flags.DirectSubmissionControllerMaxTimeout.set(200'000);
    debugManager.flags.DirectSubmissionControllerDivisor.set(1);
    debugManager.flags.DirectSubmissionControllerAdjustOnThrottleAndAcLineStatus.set(0);
    MockExecutionEnvironment executionEnvironment;
    executionEnvironment.prepareRootDeviceEnvironments(1);
    executionEnvironment.initializeMemoryManager();

    DeviceBitfield deviceBitfield(1);
    MockCommandStreamReceiver csr(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext(OsContext::create(nullptr, 0, 0,
                                                           EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                        PreemptionMode::ThreadGroup, deviceBitfield)));
    csr.setupContext(*osContext.get());

    DirectSubmissionControllerMock controller;
    controller.registerDirectSubmission(&csr);
    {
        csr.taskCount.store(1u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 1u);

        auto previousTimestamp = controller.lastTerminateCpuTimestamp;
        controller.cpuTimestamp += std::chrono::microseconds(5'000);
        controller.checkNewSubmissions();
        EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(controller.lastTerminateCpuTimestamp - previousTimestamp).count(), 5'000);
        EXPECT_TRUE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 1u);
        EXPECT_EQ(controller.timeout.count(), 5'000);
        EXPECT_EQ(controller.maxTimeout.count(), 200'000);
    }
    {
        csr.taskCount.store(2u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 2u);

        auto previousTimestamp = controller.lastTerminateCpuTimestamp;
        controller.cpuTimestamp += std::chrono::microseconds(5'500);
        controller.checkNewSubmissions();
        EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(controller.lastTerminateCpuTimestamp - previousTimestamp).count(), 5'500);
        EXPECT_TRUE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 2u);
        EXPECT_EQ(controller.timeout.count(), 8'250);
    }
    {
        csr.taskCount.store(3u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 3u);

        auto previousTimestamp = controller.lastTerminateCpuTimestamp;
        controller.cpuTimestamp += controller.maxTimeout;
        controller.checkNewSubmissions();
        EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(controller.lastTerminateCpuTimestamp - previousTimestamp).count(), controller.maxTimeout.count());
        EXPECT_TRUE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 3u);
        EXPECT_EQ(controller.timeout.count(), controller.maxTimeout.count());
    }
    {
        controller.timeout = std::chrono::microseconds(5'000);
        csr.taskCount.store(4u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 4u);

        auto previousTimestamp = controller.lastTerminateCpuTimestamp;
        controller.cpuTimestamp += controller.maxTimeout * 2;
        controller.checkNewSubmissions();
        EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(controller.lastTerminateCpuTimestamp - previousTimestamp).count(), controller.maxTimeout.count() * 2);
        EXPECT_TRUE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 4u);
        EXPECT_EQ(controller.timeout.count(), 5'000);
    }
    controller.unregisterDirectSubmission(&csr);
}

void fillTimeoutParamsMap(DirectSubmissionControllerMock &controller) {
    controller.timeoutParamsMap.clear();
    for (auto throttle : {QueueThrottle::LOW, QueueThrottle::MEDIUM, QueueThrottle::HIGH}) {
        for (auto acLineStatus : {false, true}) {
            auto key = controller.getTimeoutParamsMapKey(throttle, acLineStatus);
            TimeoutParams params{};
            params.maxTimeout = std::chrono::microseconds{500u + static_cast<size_t>(throttle) * 500u + (acLineStatus ? 0u : 1500u)};
            params.timeout = params.maxTimeout;
            params.timeoutDivisor = 1;
            params.directSubmissionEnabled = true;
            auto keyValue = std::make_pair(key, params);
            bool result = false;
            std::tie(std::ignore, result) = controller.timeoutParamsMap.insert(keyValue);
            EXPECT_TRUE(result);
        }
    }
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerAndAdjustOnThrottleAndAcLineStatusDisabledWhenSetTimeoutParamsForPlatformThenTimeoutParamsMapsIsEmpty) {
    DebugManagerStateRestore restorer;
    debugManager.flags.DirectSubmissionControllerAdjustOnThrottleAndAcLineStatus.set(0);
    MockExecutionEnvironment executionEnvironment;
    executionEnvironment.prepareRootDeviceEnvironments(1);
    executionEnvironment.initializeMemoryManager();

    DeviceBitfield deviceBitfield(1);
    MockCommandStreamReceiver csr(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext(OsContext::create(nullptr, 0, 0,
                                                           EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                        PreemptionMode::ThreadGroup, deviceBitfield)));
    csr.setupContext(*osContext.get());

    DirectSubmissionControllerMock controller;
    controller.setTimeoutParamsForPlatform(csr.getProductHelper());
    EXPECT_EQ(0u, controller.timeoutParamsMap.size());
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerAndAdjustOnThrottleAndAcLineStatusEnabledWhenThrottleOrAcLineStatusChangesThenTimeoutIsChanged) {
    DebugManagerStateRestore restorer;
    debugManager.flags.DirectSubmissionControllerDivisor.set(1);
    debugManager.flags.DirectSubmissionControllerAdjustOnThrottleAndAcLineStatus.set(1);
    MockExecutionEnvironment executionEnvironment;
    executionEnvironment.prepareRootDeviceEnvironments(1);
    executionEnvironment.initializeMemoryManager();

    DeviceBitfield deviceBitfield(1);
    MockCommandStreamReceiver csr(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext(OsContext::create(nullptr, 0, 0,
                                                           EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                        PreemptionMode::ThreadGroup, deviceBitfield)));
    csr.setupContext(*osContext.get());

    DirectSubmissionControllerMock controller;
    controller.setTimeoutParamsForPlatform(csr.getProductHelper());
    controller.registerDirectSubmission(&csr);
    EXPECT_TRUE(controller.adjustTimeoutOnThrottleAndAcLineStatus);
    fillTimeoutParamsMap(controller);

    {
        controller.lowestThrottleSubmitted = QueueThrottle::HIGH;
        csr.getLastDirectSubmissionThrottleReturnValue = QueueThrottle::LOW;
        csr.getAcLineConnectedReturnValue = true;
        csr.taskCount.store(1u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 1u);

        EXPECT_EQ(std::chrono::microseconds{500u}, controller.timeout);
        EXPECT_EQ(std::chrono::microseconds{500u}, controller.maxTimeout);
        EXPECT_EQ(1, controller.timeoutDivisor);
    }

    {
        controller.lowestThrottleSubmitted = QueueThrottle::HIGH;
        csr.getLastDirectSubmissionThrottleReturnValue = QueueThrottle::MEDIUM;
        csr.getAcLineConnectedReturnValue = true;
        csr.taskCount.store(2u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 2u);

        EXPECT_EQ(std::chrono::microseconds{1000u}, controller.timeout);
        EXPECT_EQ(std::chrono::microseconds{1000u}, controller.maxTimeout);
        EXPECT_EQ(1, controller.timeoutDivisor);
    }

    {
        controller.lowestThrottleSubmitted = QueueThrottle::HIGH;
        csr.getLastDirectSubmissionThrottleReturnValue = QueueThrottle::HIGH;
        csr.getAcLineConnectedReturnValue = true;
        csr.taskCount.store(3u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 3u);

        EXPECT_EQ(std::chrono::microseconds{1500u}, controller.timeout);
        EXPECT_EQ(std::chrono::microseconds{1500u}, controller.maxTimeout);
        EXPECT_EQ(1, controller.timeoutDivisor);
    }

    {
        controller.lowestThrottleSubmitted = QueueThrottle::HIGH;
        csr.getLastDirectSubmissionThrottleReturnValue = QueueThrottle::LOW;
        csr.getAcLineConnectedReturnValue = false;
        csr.taskCount.store(4u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 4u);

        EXPECT_EQ(std::chrono::microseconds{2000u}, controller.timeout);
        EXPECT_EQ(std::chrono::microseconds{2000u}, controller.maxTimeout);
        EXPECT_EQ(1, controller.timeoutDivisor);
    }

    {
        controller.lowestThrottleSubmitted = QueueThrottle::HIGH;
        csr.getLastDirectSubmissionThrottleReturnValue = QueueThrottle::MEDIUM;
        csr.getAcLineConnectedReturnValue = false;
        csr.taskCount.store(5u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 5u);

        EXPECT_EQ(std::chrono::microseconds{2500u}, controller.timeout);
        EXPECT_EQ(std::chrono::microseconds{2500u}, controller.maxTimeout);
        EXPECT_EQ(1, controller.timeoutDivisor);
    }

    {
        controller.lowestThrottleSubmitted = QueueThrottle::HIGH;
        csr.getLastDirectSubmissionThrottleReturnValue = QueueThrottle::HIGH;
        csr.getAcLineConnectedReturnValue = false;
        csr.taskCount.store(6u);
        controller.checkNewSubmissions();
        EXPECT_FALSE(controller.directSubmissions[&csr].isStopped);
        EXPECT_EQ(controller.directSubmissions[&csr].taskCount, 6u);

        EXPECT_EQ(std::chrono::microseconds{3000u}, controller.timeout);
        EXPECT_EQ(std::chrono::microseconds{3000u}, controller.maxTimeout);
        EXPECT_EQ(1, controller.timeoutDivisor);
    }

    {
        controller.lowestThrottleSubmitted = QueueThrottle::LOW;
        controller.checkNewSubmissions();
        EXPECT_EQ(QueueThrottle::HIGH, controller.lowestThrottleSubmitted);
    }

    controller.unregisterDirectSubmission(&csr);
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerWhenRegisterCsrsThenTimeoutIsNotAdjusted) {
    MockExecutionEnvironment executionEnvironment;
    executionEnvironment.prepareRootDeviceEnvironments(1);
    executionEnvironment.initializeMemoryManager();
    DeviceBitfield deviceBitfield(1);

    MockCommandStreamReceiver csr(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext(OsContext::create(nullptr, 0, 0,
                                                           EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                        PreemptionMode::ThreadGroup, deviceBitfield)));
    csr.setupContext(*osContext.get());

    MockCommandStreamReceiver csr1(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext1(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr1.setupContext(*osContext1.get());

    MockCommandStreamReceiver csr2(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext2(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr2.setupContext(*osContext2.get());

    MockCommandStreamReceiver csr3(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext3(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr3.setupContext(*osContext3.get());

    MockCommandStreamReceiver csr4(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext4(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr4.setupContext(*osContext4.get());

    DirectSubmissionControllerMock controller;

    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr3);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr1);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr2);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr4);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.unregisterDirectSubmission(&csr);
    controller.unregisterDirectSubmission(&csr1);
    controller.unregisterDirectSubmission(&csr2);
    controller.unregisterDirectSubmission(&csr3);
    controller.unregisterDirectSubmission(&csr4);
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerWhenRegisterCsrsFromDifferentSubdevicesThenTimeoutIsAdjusted) {
    DebugManagerStateRestore restorer;
    debugManager.flags.DirectSubmissionControllerDivisor.set(4);
    MockExecutionEnvironment executionEnvironment;
    executionEnvironment.prepareRootDeviceEnvironments(1);
    executionEnvironment.initializeMemoryManager();
    DeviceBitfield deviceBitfield(1);
    DeviceBitfield deviceBitfield1(0b10);

    MockCommandStreamReceiver csr(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext(OsContext::create(nullptr, 0, 0,
                                                           EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                        PreemptionMode::ThreadGroup, deviceBitfield)));
    csr.setupContext(*osContext.get());

    MockCommandStreamReceiver csr1(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext1(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr1.setupContext(*osContext1.get());

    MockCommandStreamReceiver csr2(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext2(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr2.setupContext(*osContext2.get());

    MockCommandStreamReceiver csr3(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext3(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr3.setupContext(*osContext3.get());

    MockCommandStreamReceiver csr4(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext4(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr4.setupContext(*osContext4.get());

    MockCommandStreamReceiver csr5(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext5(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield1)));
    csr5.setupContext(*osContext5.get());

    MockCommandStreamReceiver csr6(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext6(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield1)));
    csr6.setupContext(*osContext6.get());

    MockCommandStreamReceiver csr7(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext7(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield1)));
    csr7.setupContext(*osContext7.get());

    MockCommandStreamReceiver csr8(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext8(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield1)));
    csr8.setupContext(*osContext8.get());

    MockCommandStreamReceiver csr9(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext9(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield1)));
    csr9.setupContext(*osContext9.get());

    MockCommandStreamReceiver csr10(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext10(OsContext::create(nullptr, 0, 0,
                                                             EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                          PreemptionMode::ThreadGroup, deviceBitfield1)));
    csr10.setupContext(*osContext10.get());

    DirectSubmissionControllerMock controller;

    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr5);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr1);
    EXPECT_EQ(controller.timeout.count(), 1'250);

    controller.registerDirectSubmission(&csr2);
    EXPECT_EQ(controller.timeout.count(), 312);

    controller.registerDirectSubmission(&csr4);
    EXPECT_EQ(controller.timeout.count(), 312);

    controller.registerDirectSubmission(&csr6);
    EXPECT_EQ(controller.timeout.count(), 312);

    controller.registerDirectSubmission(&csr7);
    EXPECT_EQ(controller.timeout.count(), 312);

    controller.registerDirectSubmission(&csr9);
    EXPECT_EQ(controller.timeout.count(), 312);

    controller.registerDirectSubmission(&csr8);
    EXPECT_EQ(controller.timeout.count(), 78);

    controller.registerDirectSubmission(&csr10);
    EXPECT_EQ(controller.timeout.count(), 78);

    controller.unregisterDirectSubmission(&csr);
    controller.unregisterDirectSubmission(&csr1);
    controller.unregisterDirectSubmission(&csr2);
    controller.unregisterDirectSubmission(&csr3);
    controller.unregisterDirectSubmission(&csr4);
}

TEST(DirectSubmissionControllerTests, givenDirectSubmissionControllerDirectSubmissionControllerDivisorSetWhenRegisterCsrsThenTimeoutIsAdjusted) {
    DebugManagerStateRestore restorer;
    debugManager.flags.DirectSubmissionControllerDivisor.set(5);

    MockExecutionEnvironment executionEnvironment;
    executionEnvironment.prepareRootDeviceEnvironments(1);
    executionEnvironment.initializeMemoryManager();
    DeviceBitfield deviceBitfield(1);

    MockCommandStreamReceiver csr(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext(OsContext::create(nullptr, 0, 0,
                                                           EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                        PreemptionMode::ThreadGroup, deviceBitfield)));
    csr.setupContext(*osContext.get());

    MockCommandStreamReceiver csr1(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext1(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr1.setupContext(*osContext1.get());

    MockCommandStreamReceiver csr2(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext2(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_CCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr2.setupContext(*osContext2.get());

    MockCommandStreamReceiver csr3(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext3(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr3.setupContext(*osContext3.get());

    MockCommandStreamReceiver csr4(executionEnvironment, 0, deviceBitfield);
    std::unique_ptr<OsContext> osContext4(OsContext::create(nullptr, 0, 0,
                                                            EngineDescriptorHelper::getDefaultDescriptor({aub_stream::ENGINE_BCS, EngineUsage::regular},
                                                                                                         PreemptionMode::ThreadGroup, deviceBitfield)));
    csr4.setupContext(*osContext4.get());

    DirectSubmissionControllerMock controller;

    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr3);
    EXPECT_EQ(controller.timeout.count(), 5'000);

    controller.registerDirectSubmission(&csr1);
    EXPECT_EQ(controller.timeout.count(), 1'000);

    controller.registerDirectSubmission(&csr2);
    EXPECT_EQ(controller.timeout.count(), 200);

    controller.registerDirectSubmission(&csr4);
    EXPECT_EQ(controller.timeout.count(), 200);

    controller.unregisterDirectSubmission(&csr);
    controller.unregisterDirectSubmission(&csr1);
    controller.unregisterDirectSubmission(&csr2);
    controller.unregisterDirectSubmission(&csr3);
    controller.unregisterDirectSubmission(&csr4);
}

} // namespace NEO