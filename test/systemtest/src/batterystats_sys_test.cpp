/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "batterystats_sys_test.h"

#include <cmath>
#include <csignal>
#include <iostream>
#include <unistd.h>

#include <bt_def.h>
#include <display_power_info.h>
#include <hisysevent.h>
#include <if_system_ability_manager.h>
#include <ipc_skeleton.h>
#include <iservice_registry.h>
#include <network_search_types.h>
#include <running_lock_info.h>
#include <string_ex.h>
#include <system_ability_definition.h>
#include <wifi_hisysevent.h>


#include "battery_stats_client.h"
#include "battery_stats_service.h"
#include "stats_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::HiviewDFX;
using namespace OHOS::PowerMgr;

namespace {
static const int32_t SECOND_PER_HOUR = 3600;
static const int32_t WAIT_TIME = 1;
static std::vector<std::string> dumpArgs;
}

void BatterystatsSysTest::SetUpTestCase(void)
{
    GTEST_LOG_(INFO) << __func__;
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(true);
    dumpArgs.push_back("-batterystats");
}

void BatterystatsSysTest::TearDownTestCase(void)
{
    GTEST_LOG_(INFO) << __func__;
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.SetOnBattery(false);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(WAIT_TIME);
}

void BatterystatsSysTest::SetUp(void)
{
    GTEST_LOG_(INFO) << __func__;
}

void BatterystatsSysTest::TearDown(void)
{
    GTEST_LOG_(INFO) << __func__;
}

/**
 *
 * @tc.name: BatteryStatsSysTest_001
 * @tc.desc: test Wakelock consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_001, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_001: test start";

    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10001;
    int32_t pid = 3456;
    int32_t stateLock = 1;
    int32_t stateUnlock = 0;
    double wakelockAverage = 15;
    int32_t type = static_cast<int>(RunningLockType::RUNNINGLOCK_SCREEN);
    std::string name = " BatteryStatsSysTest_001";
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_RUNNINGLOCK", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateLock, "TYPE", type, "NAME", name);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_RUNNINGLOCK", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateUnlock, "TYPE", type, "NAME", name);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPowerMah = wakelockAverage * testTimeSec / SECOND_PER_HOUR;
    double actualPowerMah = statsClient.GetAppStatsMah(uid);
    long  actualTimeSec = statsClient.GetTotalTimeSecond(StatsUtils::STATS_TYPE_WAKELOCK_HOLD, uid);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("UID = ")
        .append(ToString(uid))
        .append(", PID = ")
        .append(ToString(pid))
        .append(", wakelock type = ")
        .append(ToString(type))
        .append(", wakelock name = ")
        .append(name)
        .append(", wakelock state = ")
        .append("UNLOCK");

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);

    GTEST_LOG_(INFO) << __func__ << ": expected debug info: " << expectedDebugInfo;
    GTEST_LOG_(INFO) << __func__ << ": actual debug info: " << actualDebugInfo;

    auto index = actualDebugInfo.find(expectedDebugInfo);

    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << testTimeSec << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTimeSec << " seconds";

    EXPECT_LE(abs(expectedPowerMah - actualPowerMah), deviation)
        << " BatteryStatsSysTest_001 fail due to consumption deviation larger than 0.01";
    EXPECT_EQ(testTimeSec,  actualTimeSec) << " BatteryStatsSysTest_001 fail due to wakelock time";
    EXPECT_TRUE(index != string::npos) << " BatteryStatsSysTest_001 fail due to not found Wakelock related debug info";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_001: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_002
 * @tc.desc: test Screen consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_002, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_002: test start";

    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t stateOn = static_cast<int>(DisplayPowerMgr::DisplayState::DISPLAY_ON);
    int32_t stateOff = static_cast<int>(DisplayPowerMgr::DisplayState::DISPLAY_OFF);
    int32_t brightnessBefore = 150;
    int32_t brightnessAfter = 100;
    double screenOnAverage = 90;
    double screenBrightnessAverage = 2;
    double deviation = 0.1;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_SCREEN", HiSysEvent::EventType::STATISTIC, "STATE", stateOn,
        "BRIGHTNESS", brightnessBefore);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_SCREEN", HiSysEvent::EventType::STATISTIC, "STATE", stateOff,
        "BRIGHTNESS", brightnessAfter);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double average = screenBrightnessAverage * brightnessBefore + screenOnAverage;

    double expectedPowerMah = average * testTimeSec / SECOND_PER_HOUR;
    double actualPowerMah = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_SCREEN);
    long actualTimeSec = statsClient.GetTotalTimeSecond(StatsUtils::STATS_TYPE_SCREEN_ON);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("Screen is in off state, brigntness level = ")
        .append(ToString(brightnessAfter));

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);

    GTEST_LOG_(INFO) << __func__ << ": expected debug info: " << expectedDebugInfo;
    GTEST_LOG_(INFO) << __func__ << ": actual debug info: " << actualDebugInfo;

    auto index = actualDebugInfo.find(expectedDebugInfo);

    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPowerMah << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": expected time = " << testTimeSec << " seconds";
    GTEST_LOG_(INFO) << __func__ << ": actual time = " <<  actualTimeSec << " seconds";

    EXPECT_LE((abs(expectedPowerMah - actualPowerMah)) / expectedPowerMah, deviation)
        << " BatteryStatsSysTest_002 fail due to consumption deviation larger than 0.01";
    EXPECT_EQ(testTimeSec,  actualTimeSec) << " BatteryStatsSysTest_002 fail due to wakelock time";
    EXPECT_TRUE(index != string::npos) << " BatteryStatsSysTest_003 fail due to not found battery related debug info";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_002: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_003
 * @tc.desc: test Battery stats event
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_003, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_003: test start";

    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    long testWaitTimeSec = 1;
    int32_t batteryLevel = 60;
    int32_t batteryCurrent = 30;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_BATTERY", HiSysEvent::EventType::STATISTIC, "BATTERY_LEVEL",
        batteryLevel, "CURRENT_NOW", batteryCurrent);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("Battery level = ")
        .append(ToString(batteryLevel))
        .append(", current now = ")
        .append(ToString(batteryCurrent))
        .append("ma");

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);

    GTEST_LOG_(INFO) << __func__ << ": expected debug info: " << expectedDebugInfo;
    GTEST_LOG_(INFO) << __func__ << ": actual debug info: " << actualDebugInfo;

    auto index = actualDebugInfo.find(expectedDebugInfo);

    EXPECT_TRUE(index != string::npos) << " BatteryStatsSysTest_003 fail due to not found battery related debug info";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_003: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_004
 * @tc.desc: test Thermal stats event
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_004, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_004: test start";

    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    long testWaitTimeSec = 1;
    std::string partName = "Battery";
    int32_t temperature = 40;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_TEMPERATURE", HiSysEvent::EventType::STATISTIC, "NAME",
        partName, "TEMPERATURE", temperature);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("Part name = ")
        .append(partName)
        .append(", temperature = ")
        .append(ToString(temperature))
        .append("degrees Celsius");

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);

    GTEST_LOG_(INFO) << __func__ << ": expected debug info: " << expectedDebugInfo;
    GTEST_LOG_(INFO) << __func__ << ": actual debug info: " << actualDebugInfo;

    auto index = actualDebugInfo.find(expectedDebugInfo);

    EXPECT_TRUE(index != string::npos) << " BatteryStatsSysTest_004 fail due to not found thermal related debug info";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_004: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_005
 * @tc.desc: test WorkScheduler stats event
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_005, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_005: test start";

    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    long testWaitTimeSec = 1;
    int32_t pid = 3457;
    int32_t uid = 10002;
    int32_t type = 1;
    int32_t interval = 30000;
    int32_t state = 5;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_WORKSCHEDULER", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "TYPE", type, "INTERVAL", interval, "STATE", state);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    std::string expectedDebugInfo;
    expectedDebugInfo.append("UID = ")
        .append(ToString(uid))
        .append(", PID = ")
        .append(ToString(pid))
        .append(", work type = ")
        .append(ToString(type))
        .append(", work interval = ")
        .append(ToString(interval))
        .append(", work state = ")
        .append(ToString(state));

    std::string actualDebugInfo = statsClient.Dump(dumpArgs);

    GTEST_LOG_(INFO) << __func__ << ": expected debug info: " << expectedDebugInfo;
    GTEST_LOG_(INFO) << __func__ << ": actual debug info: " << actualDebugInfo;

    auto index = actualDebugInfo.find(expectedDebugInfo);

    EXPECT_TRUE(index != string::npos) << " BatteryStatsSysTest_005 fail due to no WorkScheduler debug info found";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_005: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_006
 * @tc.desc: test Dump function
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_006, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_006: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    std::string result;
    result.clear();
    result = statsClient.Dump(dumpArgs);
    EXPECT_TRUE(result != "") << " BatteryStatsSysTest_006 fail due to nothing";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_006: test end";
    statsClient.Reset();
}

/**
 *
 * @tc.name: BatteryStatsSysTest_007
 * @tc.desc: test Radio consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_007, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_007: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t stateScan = static_cast<int>(Telephony::RegServiceState::REG_STATE_SEARCH);
    int32_t stateInService = static_cast<int>(Telephony::RegServiceState::REG_STATE_IN_SERVICE);
    int32_t signalBefore = 4;
    int32_t signalAfter = 0;
    double radioLevel4OnAverage = 470;
    double radioLevel0OnAverage = 160;
    double radioScanAverage = 30;
    double deviation = 0.1;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_RADIO", HiSysEvent::EventType::STATISTIC, "STATE", stateScan,
        "SIGNAL", signalBefore);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_RADIO", HiSysEvent::EventType::STATISTIC, "STATE",
        stateInService, "SIGNAL", signalAfter);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedLevelPower = radioLevel4OnAverage * testTimeSec + radioLevel0OnAverage * testWaitTimeSec;
    double expectedScanPower = radioScanAverage * testTimeSec;
    double expectedPower = (expectedLevelPower + expectedScanPower) / SECOND_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_RADIO);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE((abs(expectedPower - actualPower)) / expectedPower, deviation)
        << " BatteryStatsSysTest_007 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_007: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_008
 * @tc.desc: test Bluetooth consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_008, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_008: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double bluetoothOnAverageMa = 1;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t stateOn = static_cast<int>(bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int>(bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;
    double deviation = 0.01;
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "BLUETOOTH_BR_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "BR_STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "BLUETOOTH_BR_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "BR_STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * bluetoothOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_008 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_008: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_009
 * @tc.desc: test Wifi consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_009, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_009: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double wifiOnAverageMa = 83;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t stateOn = static_cast<int32_t>(Wifi::WifiOperType::ENABLE);
    int32_t stateOff = static_cast<int32_t>(Wifi::WifiOperType::DISABLE);
    double deviation = 0.01;
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "WIFI_STATE", HiSysEvent::EventType::STATISTIC, "OPER_TYPE",
        stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "WIFI_STATE", HiSysEvent::EventType::STATISTIC, "OPER_TYPE",
        stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * wifiOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_009 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_009: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_010
 * @tc.desc: test Phone consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_010, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_010: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double phoneOnAverageMa = 50;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_PHONE", HiSysEvent::EventType::STATISTIC, "STATE",
        stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_PHONE", HiSysEvent::EventType::STATISTIC, "STATE",
        stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * phoneOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_010 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_010: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_011
 * @tc.desc: test Idle consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_011, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_011: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_IDLE);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE) << " BatteryStatsSysTest_011 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_011: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_012
 * @tc.desc: test User consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_012, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_012: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    int32_t uerId = 20003;

    double actualPower = statsClient.GetAppStatsMah(uerId);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE) << " BatteryStatsSysTest_012 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_012: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_013
 * @tc.desc: test Audio consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_013, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_013: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double audioOnAverageMa = 85;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_AUDIO", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_AUDIO", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * audioOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_013 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_013: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_014
 * @tc.desc: test Gps consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_014, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_014: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double gpsOnAverageMa = 80;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10003;
    int32_t pid = 3458;
    std::string stateOn = "start";
    std::string stateOff = "stop";
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "GNSS_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "GNSS_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * gpsOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_014 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_014: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_015
 * @tc.desc: test Sensor consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_015, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_015: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double sensorGravityOnAverageMa = 15;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_SENSOR_GRAVITY", HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_SENSOR_GRAVITY", HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * sensorGravityOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_015 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_015: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_016
 * @tc.desc: test Camera consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_016, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_016: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double cameraOnAverageMa = 810;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_CAMERA", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_CAMERA", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * cameraOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_016 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_016: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_017
 * @tc.desc: test Flashlight consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_017, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_017: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double flashlightOnAverageMa = 320;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_FLASHLIGHT", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_FLASHLIGHT", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * flashlightOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_017 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_017: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_018
 * @tc.desc: test Bluetooth and Wifi consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_018, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_018: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double bluetoothOnAverageMa = 1;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t stateOn = static_cast<int>(bluetooth::BTStateID::STATE_TURN_ON);
    int32_t stateOff = static_cast<int>(bluetooth::BTStateID::STATE_TURN_OFF);
    int32_t uid = 10003;
    int32_t pid = 3458;
    double deviation = 0.01;
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "BLUETOOTH_BR_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "BR_STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "BLUETOOTH_BR_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "BR_STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * bluetoothOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_BLUETOOTH);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_018 fail due to power mismatch";

    double wifiOnAverageMa = 83;
    stateOn = static_cast<int32_t>(Wifi::WifiOperType::ENABLE);
    stateOff = static_cast<int32_t>(Wifi::WifiOperType::DISABLE);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "WIFI_STATE", HiSysEvent::EventType::STATISTIC, "OPER_TYPE",
        stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "WIFI_STATE", HiSysEvent::EventType::STATISTIC, "OPER_TYPE",
        stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    expectedPower = testTimeSec * wifiOnAverageMa / SECOND_PER_HOUR;
    actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_WIFI);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_018 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_018: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_019
 * @tc.desc: test Flashlight and Camera consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_019, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_019: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double flashlightOnAverageMa = 320;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_FLASHLIGHT", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_FLASHLIGHT", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * flashlightOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_019 fail due to power mismatch";

    double cameraOnAverageMa = 810;
    uid = 10004;
    pid = 3459;
    stateOn = 1;
    stateOff = 0;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_CAMERA", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_CAMERA", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    expectedPower = testTimeSec * cameraOnAverageMa / SECOND_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_019 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_019: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_020
 * @tc.desc: test Audio, Sensor and Gps consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_020, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_020: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double audioOnAverageMa = 85;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t uid = 10003;
    int32_t pid = 3458;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_AUDIO", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_AUDIO", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * audioOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetAppStatsMah(uid);
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_020 fail due to power mismatch";

    double gpsOnAverageMa = 80;
    uid = 10004;
    pid = 3459;
    std::string gpsStateOn = "start";
    std::string gpsStateOff = "stop";

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "GNSS_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", gpsStateOn);
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "GNSS_STATE", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", gpsStateOff);
    sleep(testWaitTimeSec);

    expectedPower = testTimeSec * gpsOnAverageMa / SECOND_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_020 fail due to power mismatch";

    double sensorGravityOnAverageMa = 15;
    uid = 10005;
    pid = 3457;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_SENSOR_GRAVITY", HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOn);
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_SENSOR_GRAVITY", HiSysEvent::EventType::STATISTIC, "PID",
        pid, "UID", uid, "STATE", stateOff);
    sleep(testWaitTimeSec);

    expectedPower = testTimeSec * sensorGravityOnAverageMa / SECOND_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_020 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_020: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_021
 * @tc.desc: test Phone and Audio consumption
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_021, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_021: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double phoneOnAverageMa = 50;
    long testTimeSec = 2;
    long testWaitTimeSec = 1;
    int32_t stateOn = 1;
    int32_t stateOff = 0;
    double deviation = 0.01;
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_PHONE", HiSysEvent::EventType::STATISTIC, "STATE",
        stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_PHONE", HiSysEvent::EventType::STATISTIC, "STATE",
        stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    double expectedPower = testTimeSec * phoneOnAverageMa / SECOND_PER_HOUR;
    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_PHONE);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_021 fail due to power mismatch";

    double audioOnAverageMa = 85;
    int32_t uid = 10003;
    int32_t pid = 3458;
    stateOn = 1;
    stateOff = 0;

    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_AUDIO", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOn);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 2 seconds";
    sleep(testTimeSec);
    HiSysEvent::Write(HiSysEvent::Domain::POWERMGR, "POWER_AUDIO", HiSysEvent::EventType::STATISTIC, "PID", pid,
        "UID", uid, "STATE", stateOff);
    GTEST_LOG_(INFO) << __func__ << ": Sleep 1 seconds";
    sleep(testWaitTimeSec);

    expectedPower = testTimeSec * audioOnAverageMa / SECOND_PER_HOUR;
    actualPower = statsClient.GetAppStatsMah(uid);
    GTEST_LOG_(INFO) << __func__ << ": expected consumption = " << expectedPower << " mAh";
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_LE(abs(expectedPower - actualPower), deviation) << " BatteryStatsSysTest_021 fail due to power mismatch";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_021: test end";
}

/**
 *
 * @tc.name: BatteryStatsSysTest_022
 * @tc.desc: test Idle and consumption, Dump function
 * @tc.type: FUNC
 */
HWTEST_F (BatterystatsSysTest,  BatteryStatsSysTest_022, TestSize.Level0)
{
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_022: test start";
    auto& statsClient = BatteryStatsClient::GetInstance();
    statsClient.Reset();

    double actualPower = statsClient.GetPartStatsMah(BatteryStatsInfo::CONSUMPTION_TYPE_IDLE);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";

    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE) << " BatteryStatsSysTest_022 fail due to power mismatch";

    int32_t uerId = 20003;

    actualPower = statsClient.GetAppStatsMah(uerId);
    GTEST_LOG_(INFO) << __func__ << ": actual consumption = " << actualPower << " mAh";
    EXPECT_TRUE(actualPower >= StatsUtils::DEFAULT_VALUE) << " BatteryStatsSysTest_022 fail due to power mismatch";

    std::string result;
    result.clear();
    result = statsClient.Dump(dumpArgs);
    EXPECT_TRUE(result != "") << " BatteryStatsSysTest_022 fail due to nothing";
    GTEST_LOG_(INFO) << " BatteryStatsSysTest_022: test end";
}