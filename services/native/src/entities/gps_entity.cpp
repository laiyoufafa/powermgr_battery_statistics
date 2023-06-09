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

#include "entities/gps_entity.h"

#include "battery_stats_service.h"
#include "stats_hilog_wrapper.h"

namespace OHOS {
namespace PowerMgr {
namespace {
    auto g_statsService = DelayedStatsSpSingleton<BatteryStatsService>::GetInstance();
}

GpsEntity::GpsEntity()
{
    consumptionType_ = BatteryStatsInfo::CONSUMPTION_TYPE_GPS;
}

long GpsEntity::GetActiveTimeMs(int32_t uid, StatsUtils::StatsType statsType, int16_t level)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    long activeTimeMs = StatsUtils::DEFAULT_VALUE;
    if (statsType == StatsUtils::STATS_TYPE_GPS_ON) {
        auto iter = gpsTimerMap_.find(uid);
        if (iter != gpsTimerMap_.end()) {
            activeTimeMs = iter->second->GetRunningTimeMs();
            STATS_HILOGI(STATS_MODULE_SERVICE, "Got gps on time: %{public}ldms for uid: %{public}d", activeTimeMs,
                uid);
        } else {
            STATS_HILOGE(STATS_MODULE_SERVICE, "Didn't find related timer for uid: %{public}d, return 0", uid);
        }
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return activeTimeMs;
}

void GpsEntity::Calculate(int32_t uid)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    auto gpsOnAverageMa = g_statsService->GetBatteryStatsParser()->GetAveragePowerMa(StatsUtils::CURRENT_GPS_ON);
    auto gpsOnTimeMs = GetActiveTimeMs(uid, StatsUtils::STATS_TYPE_GPS_ON);
    auto gpsOnPowerMah = gpsOnAverageMa * gpsOnTimeMs / StatsUtils::MS_IN_HOUR;
    auto iter = gpsPowerMap_.find(uid);
    if (iter != gpsPowerMap_.end()) {
        STATS_HILOGI(STATS_MODULE_SERVICE, "Update gps on power consumption: %{public}lfmAh for uid: %{public}d",
            gpsOnAverageMa, uid);
        iter->second = gpsOnPowerMah;
    } else {
        STATS_HILOGI(STATS_MODULE_SERVICE, "Create gps on power consumption: %{public}lfmAh for uid: %{public}d",
            gpsOnAverageMa, uid);
        gpsPowerMap_.insert(std::pair<int32_t, double>(uid, gpsOnPowerMah));
    }

    STATS_HILOGI(STATS_MODULE_SERVICE, "Calculate gps on average power: %{public}lfma", gpsOnAverageMa);
    STATS_HILOGI(STATS_MODULE_SERVICE, "Calculate gps on time: %{public}ldms for uid: %{public}d", gpsOnTimeMs,
        uid);
    STATS_HILOGI(STATS_MODULE_SERVICE, "Calculate gps on power consumption: %{public}lfmAh for uid: %{public}d",
        gpsOnPowerMah, uid);
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
}

double GpsEntity::GetEntityPowerMah(int32_t uidOrUserId)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    double power = StatsUtils::DEFAULT_VALUE;
    auto iter = gpsPowerMap_.find(uidOrUserId);
    if (iter != gpsPowerMap_.end()) {
        power = iter->second;
        STATS_HILOGI(STATS_MODULE_SERVICE, "Got app gps power consumption: %{public}lfmAh for uid: %{public}d",
            power, uidOrUserId);
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE,
            "No app gps power consumption related with uid: %{public}d found, return 0", uidOrUserId);
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return power;
}

double GpsEntity::GetStatsPowerMah(StatsUtils::StatsType statsType, int32_t uid)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    double power = StatsUtils::DEFAULT_VALUE;
    if (statsType == StatsUtils::STATS_TYPE_GPS_ON) {
        auto gpsOnIter = gpsPowerMap_.find(uid);
        if (gpsOnIter != gpsPowerMap_.end()) {
            power = gpsOnIter->second;
            STATS_HILOGI(STATS_MODULE_SERVICE, "Got gps on power consumption: %{public}lfmAh for uid: %{public}d",
                power, uid);
        } else {
            STATS_HILOGE(STATS_MODULE_SERVICE,
                "No gps on power consumption related with uid: %{public}d found, return 0", uid);
        }
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return power;
}

std::shared_ptr<StatsHelper::ActiveTimer> GpsEntity::GetOrCreateTimer(int32_t uid, StatsUtils::StatsType statsType,
    int16_t level)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    if (statsType == StatsUtils::STATS_TYPE_GPS_ON) {
        auto gpsOnIter = gpsTimerMap_.find(uid);
        if (gpsOnIter != gpsTimerMap_.end()) {
            STATS_HILOGI(STATS_MODULE_SERVICE, "Got gps on timer for uid: %{public}d", uid);
            STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
            return gpsOnIter->second;
        } else {
            STATS_HILOGE(STATS_MODULE_SERVICE, "Create gps on timer for uid: %{public}d", uid);
            std::shared_ptr<StatsHelper::ActiveTimer> timer = std::make_shared<StatsHelper::ActiveTimer>();
            gpsTimerMap_.insert(std::pair<int32_t, std::shared_ptr<StatsHelper::ActiveTimer>>(uid, timer));
            STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
            return timer;
        }
    } else {
        STATS_HILOGI(STATS_MODULE_SERVICE, "Create active timer failed");
        return nullptr;
    }
}

void GpsEntity::Reset()
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    // Reset app Gps on total power consumption
    for (auto &iter : gpsPowerMap_) {
        iter.second = StatsUtils::DEFAULT_VALUE;
    }

    // Reset Gps on timer
    for (auto &iter : gpsTimerMap_) {
        if (iter.second) {
            iter.second->Reset();
        }
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
}
} // namespace PowerMgr
} // namespace OHOS