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

#ifndef IBATTERY_STATS_H
#define IBATTERY_STATS_H

#include "battery_stats_info.h"

#include "iremote_broker.h"
#include "iremote_object.h"

namespace OHOS {
namespace PowerMgr {
class IBatteryStats : public IRemoteBroker {
public:
    enum {
        BATTERY_STATS_GET = 0,
        BATTERY_STATS_GETAPPMAH,
        BATTERY_STATS_GETAPPPER,
        BATTERY_STATS_GETPARTMAH,
        BATTERY_STATS_GETPARTPER,
        BATTERY_STATS_GETTIME,
        BATTERY_STATS_GETDATA,
        BATTERY_STATS_RESET
    };
    virtual BatteryStatsInfoList GetBatteryStats() = 0;
    virtual double GetAppStatsMah(const int32_t& uid) = 0;
    virtual double GetAppStatsPercent(const int32_t& uid) = 0;
    virtual double GetPartStatsMah(const BatteryStatsInfo::BatteryStatsType& type) = 0;
    virtual double GetPartStatsPercent(const BatteryStatsInfo::BatteryStatsType& type) = 0;
    virtual uint64_t GetTotalTimeSecond(const std::string& hwId, const int32_t& uid) = 0;
    virtual uint64_t GetTotalDataBytes(const std::string& hwId, const int32_t& uid) = 0;
    virtual void Reset() = 0;
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.powermgr.IBatteryStats");
};
} // namespace PowerMgr
} // namespace OHOS

#endif // IBATTERY_STATS_H
