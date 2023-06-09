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

#ifndef WIFI_ENTITY_H
#define WIFI_ENTITY_H

#include <map>

#include "entities/battery_stats_entity.h"

namespace OHOS {
namespace PowerMgr {
class WifiEntity : public BatteryStatsEntity {
public:
    WifiEntity();
    ~WifiEntity() = default;
    void Calculate(int32_t uid = StatsUtils::INVALID_VALUE) override;
    long GetActiveTimeMs(StatsUtils::StatsType statsType, int16_t level = StatsUtils::INVALID_VALUE) override;
    long GetActiveTimeMs(int32_t uid, StatsUtils::StatsType statsType,
        int16_t level = StatsUtils::INVALID_VALUE) override;
    double GetEntityPowerMah(int32_t uidOrUserId = StatsUtils::INVALID_VALUE) override;
    double GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid = StatsUtils::INVALID_VALUE) override;
    void Reset() override;
    void DumpInfo(std::string& result, int32_t uid = StatsUtils::INVALID_VALUE) override;
    long GetTrafficByte(StatsUtils::StatsType statsType, int32_t uid = StatsUtils::INVALID_VALUE) override;
    std::shared_ptr<StatsHelper::ActiveTimer> GetOrCreateTimer(StatsUtils::StatsType statsType,
        int16_t level = StatsUtils::INVALID_VALUE) override;
    std::shared_ptr<StatsHelper::ActiveTimer> GetOrCreateTimer(int32_t uid, StatsUtils::StatsType statsType,
        int16_t level = StatsUtils::INVALID_VALUE) override;
    std::shared_ptr<StatsHelper::Counter> GetOrCreateCounter(StatsUtils::StatsType statsType,
        int32_t uid = StatsUtils::INVALID_VALUE) override;
protected:

private:
    double GetWifiUidPower();
    void CalculateWifiPower();
    void CalculateWifiPowerForApp(int32_t uid);
    double wifiPowerMah_ = StatsUtils::DEFAULT_VALUE;
    std::map<int32_t, double> appWifiPowerMap_;
    std::map<int32_t, double> appWifiScanPowerMap_;
    std::map<int32_t, double> appWifiRxPowerMap_;
    std::map<int32_t, double> appWifiTxPowerMap_;
    std::shared_ptr<StatsHelper::ActiveTimer> wifiOnTimer_;
    std::map<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>> appWifiScanTimerMap_;
    std::map<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>> appWifiRxTimerMap_;
    std::map<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>> appWifiTxTimerMap_;
    std::map<int32_t, std::shared_ptr<StatsHelper::Counter>> appWifiRxCounterMap_;
    std::map<int32_t, std::shared_ptr<StatsHelper::Counter>> appWifiTxCounterMap_;
};
} // namespace PowerMgr
} // namespace OHOS
#endif // WIFI_ENTITY_H