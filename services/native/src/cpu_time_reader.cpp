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

#include "cpu_time_reader.h"

#include "battery_stats_service.h"
#include "battery_stats_utils.h"
#include "stats_hilog_wrapper.h"
#include "string_ex.h"
#include "time_helper.h"

namespace OHOS {
namespace PowerMgr {
namespace {
static const std::string UID_CPU_ACTIVE_TIME_FILE = "/proc/uid_concurrent_active_time";
static const std::string UID_CPU_CLUSTER_TIME_FILE = "/proc/uid_concurrent_policy_time";
static const std::string UID_CPU_FREQ_TIME_FILE = "/proc/uid_time_in_state";
static const std::string UID_CPU_TIME_FILE = "/proc/uid_cputime/show_uid_stat";
} // namespace
bool CpuTimeReader::Init()
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");

    if (!UpdateCpuTime()) {
        STATS_HILOGE(STATS_MODULE_SERVICE, "Initialization failed: update cpu time failed");
        return false;
    }

    STATS_HILOGI(STATS_MODULE_SERVICE, "Initialization succeeded");
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return true;
}

long CpuTimeReader::GetUidCpuActiveTimeMs(int32_t uid)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    long cpuActiveTime = 0;
    auto iter = activeTimeMap_.find(uid);
    if (iter != activeTimeMap_.end()) {
        cpuActiveTime = iter->second;
        STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu active time: %{public}ld for uid: %{public}d",
            cpuActiveTime, uid);
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "No cpu active time found for uid: %{public}d, return 0", uid);
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return cpuActiveTime;
}

void CpuTimeReader::DumpInfo(std::string& result)
{
    for (auto uidIter = uidTimeMap_.begin(); uidIter != uidTimeMap_.end(); uidIter++) {
        std::string freqTime = "";
        auto freqIter = freqTimeMap_.find(uidIter->first);
        if (freqIter != freqTimeMap_.end()) {
            for (auto timeIter = freqIter->second.begin(); timeIter != freqIter->second.end(); timeIter++) {
                for (uint32_t i = 0; i < timeIter->second.size(); i++) {
                    freqTime.append(ToString(timeIter->second[i]))
                        .append(" ");
                }
            }
        }
        result.append(ToString(uidIter->first))
            .append(":\n");
        result.append("Total cpu time: userSpaceTime=")
            .append(ToString(uidIter->second[0]))
            .append("ms, systemSpaceTime=")
            .append(ToString(uidIter->second[1]))
            .append("ms\n");
        result.append("Total cpu time per freq: ")
            .append(freqTime)
            .append("\n");
    }
}

long CpuTimeReader::GetUidCpuClusterTimeMs(int32_t uid, uint32_t cluster)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    long cpuClusterTime = 0;
    auto iter = clusterTimeMap_.find(uid);
    if (iter != clusterTimeMap_.end()) {
        auto cpuClusterTimeVector = iter->second;
        STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu cluster time vector for uid: %{public}d, size: %{public}d", uid,
            cpuClusterTimeVector.size());
        if (cluster < cpuClusterTimeVector.size()) {
            cpuClusterTime = cpuClusterTimeVector[cluster];
            STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu cluster time: %{public}ld of cluster: %{public}d",
                cpuClusterTime, cluster);
        } else {
            STATS_HILOGE(STATS_MODULE_SERVICE, "No cpu cluster time of cluster: %{public}d found, return 0", cluster);
        }
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "No cpu cluster time vector found for uid: %{public}d, return 0", uid);
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return cpuClusterTime;
}

long CpuTimeReader::GetUidCpuFreqTimeMs(int32_t uid, uint32_t cluster, uint32_t speed)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    long cpuFreqTime = 0;
    auto uidIter = freqTimeMap_.find(uid);
    if (uidIter != freqTimeMap_.end()) {
        auto cpuFreqTimeMap = uidIter->second;
        STATS_HILOGD(STATS_MODULE_SERVICE, "Got uid cpu freq time map for uid: %{public}d, size: %{public}d", uid,
            cpuFreqTimeMap.size());
        auto clusterIter = cpuFreqTimeMap.find(cluster);
        if (clusterIter != cpuFreqTimeMap.end()) {
            auto cpuFreqTimeVector = clusterIter->second;
            STATS_HILOGD(STATS_MODULE_SERVICE, "Got cluster cpu freq time vector of cluster: %{public}d,\
                size: %{public}d", cluster, cpuFreqTimeVector.size());
            if (speed < cpuFreqTimeVector.size()) {
                cpuFreqTime = cpuFreqTimeVector[speed];
                STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu freq time: %{public}ld of speed: %{public}d", cpuFreqTime,
                    speed);
            } else {
                STATS_HILOGE(STATS_MODULE_SERVICE, "No cpu freq time of speed: %{public}d found, return 0", speed);
            }
        } else {
            STATS_HILOGE(STATS_MODULE_SERVICE, "No cluster cpu freq time vector of cluster: %{public}d found,\
                return 0", cluster);
        }
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "No uid cpu freq time map found for uid: %{public}d, return 0", uid);
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return cpuFreqTime;
}

std::vector<long> CpuTimeReader::GetUidCpuTimeMs(int32_t uid)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    std::vector<long> cpuTimeVec;
    auto iter = uidTimeMap_.find(uid);
    if (iter != uidTimeMap_.end()) {
        cpuTimeVec = iter->second;
        STATS_HILOGD(STATS_MODULE_SERVICE, "Got uid cpu time vector for uid: %{public}d, size: %{public}d", uid,
            cpuTimeVec.size());
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "No uid cpu time vector found for uid: %{public}d, return null", uid);
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return cpuTimeVec;
}

bool CpuTimeReader::UpdateCpuTime()
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    bool result = true;
    if (ReadUidCpuClusterTime()) {
        STATS_HILOGD(STATS_MODULE_SERVICE, "Read uid cpu cluster time successfully");
    } else {
        result = false;
        STATS_HILOGE(STATS_MODULE_SERVICE, "Read uid cpu cluster time failed");
    }

    if (ReadUidCpuTime()) {
        STATS_HILOGD(STATS_MODULE_SERVICE, "Read uid cpu time successfully");
    } else {
        result = false;
        STATS_HILOGE(STATS_MODULE_SERVICE, "Read uid cpu time failed");
    }

    if (ReadUidCpuActiveTime()) {
        STATS_HILOGD(STATS_MODULE_SERVICE, "Read uid cpu active time successfully");
    } else {
        result = false;
        STATS_HILOGE(STATS_MODULE_SERVICE, "Read uid cpu active time failed");
    }

    if (ReadUidCpuFreqTime()) {
        STATS_HILOGD(STATS_MODULE_SERVICE, "Read uid cpu freq time successfully");
    } else {
        result = false;
        STATS_HILOGE(STATS_MODULE_SERVICE, "Read uid cpu freq time failed");
    }

    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
    return result;
}

bool CpuTimeReader::ReadUidCpuActiveTime() {
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    std::ifstream input(UID_CPU_ACTIVE_TIME_FILE);
    if (input) {
        std::string line;
        int32_t uid = -1;
        long timeMs;
        while (getline(input, line)) {
            timeMs = 0;
            std::vector<std::string> splitedLine;
            split(line, ':', splitedLine);
            if (splitedLine[0] == "cpus") {
                continue;
            } else {
                uid = stoi(splitedLine[0]);
            }

            std::vector<std::string> splitedTime;
            split(splitedLine[1], ' ', splitedTime);
            for (uint16_t i = 0; i < splitedTime.size(); i++) {
                timeMs += stol(splitedTime[i]) * 10; // Unit is 10ms
            }

            long increment = 0;
            if (timeMs > 0) {
                auto iterLast = lastActiveTimeMap_.find(uid);
                if (iterLast != lastActiveTimeMap_.end()) {
                    increment = timeMs - iterLast->second;
                    if (increment > 0) {
                        iterLast->second = timeMs;
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu active time increment: %{public}ld ms\
                            for uid: %{public}d", increment, uid);
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Update last cpu active time to: %{public}ld ms\
                            for uid: %{public}d", timeMs, uid);
                    } else {
                        STATS_HILOGE(STATS_MODULE_SERVICE, "Negative cpu active time increment got");
                        return false;
                    }
                } else {
                    lastActiveTimeMap_.insert(std::pair<int32_t, long>(uid, timeMs));
                    increment = timeMs;
                    STATS_HILOGD(STATS_MODULE_SERVICE, "Add last cpu active time as: %{public}ld ms\
                        for uid: %{public}d", increment, uid);
                }
            }

            if (TimeHelper::IsOnBattery() || BatteryStatsUtils::DEBUG) { // Just for debug
                auto iter = activeTimeMap_.find(uid);
                if (iter != activeTimeMap_.end()) {
                    iter->second += increment;
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Update cpu active time to: %{public}ld ms\
                            for uid: %{public}d", iter->second, uid);
                } else {
                    activeTimeMap_.insert(std::pair<int32_t, long>(uid, increment));
                    STATS_HILOGD(STATS_MODULE_SERVICE, "Add cpu active time as: %{public}ld ms for uid: %{public}d",
                        increment, uid);
                }
            } else {
                STATS_HILOGD(STATS_MODULE_SERVICE, "Power supply is connected, don't add the increment");
            }
        }
        STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
        return true;
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "Opening file: %{public}s failed", UID_CPU_ACTIVE_TIME_FILE.c_str());
        return false;
    }
}

bool CpuTimeReader::ReadUidCpuClusterTime() {
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    std::ifstream input(UID_CPU_CLUSTER_TIME_FILE);
    if (input) {
        std::string line;
        int32_t uid = -1;
        std::vector<uint16_t> clusters;
        std::vector<long> clusterTime;
        while (getline(input, line)) {
            clusterTime.clear();
            if (line.find("policy") != line.npos) {
                std::vector<std::string> splitedPolicy;
                split(line, ' ', splitedPolicy);
                for (uint16_t i = 0; i < splitedPolicy.size(); i += 2) {
                    uint16_t coreNum = stoi(splitedPolicy[i+1]);
                    clusters.push_back(coreNum);
                    clustersMap_.insert(std::pair<uint16_t, uint16_t>(i, coreNum));
                    STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu core num: %{public}d", coreNum);
                }
                continue;
            }

            std::vector<std::string> splitedLine;
            split(line, ':', splitedLine);
            uid = stoi(splitedLine[0]);

            std::vector<std::string> splitedTime;
            split(splitedLine[1], ' ', splitedTime);
            uint16_t count = 0;
            for (uint16_t i = 0; i < clusters.size(); i++) {
                long tempTimeMs = 0;
                for (int j = 0; j < clusters[i]; j++) {
                    tempTimeMs += stol(splitedTime[count++]) * 10; // Unit is 10ms
                }
                STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu cluster time: %{public}ld", tempTimeMs);
                clusterTime.push_back(tempTimeMs);
            }

            std::vector<long> increments;
            auto iterLast = lastClusterTimeMap_.find(uid);
            if (iterLast != lastClusterTimeMap_.end()) {
                for (uint16_t i = 0; i < clusters.size(); i++) {
                    long increment = clusterTime[i] - iterLast->second[i];
                    if (increment > 0) {
                        iterLast->second[i] = clusterTime[i];
                        increments.push_back(increment);
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu cluster time increment: %{public}ld ms,\
                            uid: %{public}d", increment, uid);
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Update last cpu cluster time to: %{public}ld ms,\
                            uid: %{public}d", clusterTime[i], uid);
                    } else {
                        STATS_HILOGE(STATS_MODULE_SERVICE, "Negative cpu cluster time increment got");
                        return false;
                    }
                }
            } else {
                lastClusterTimeMap_.insert(std::pair<int32_t, std::vector<long>>(uid, clusterTime));
                increments = clusterTime;
                STATS_HILOGD(STATS_MODULE_SERVICE, "Add last cpu cluster time for uid: %{public}d", uid);
            }

            if (TimeHelper::IsOnBattery() || BatteryStatsUtils::DEBUG) { // Just for debug
                auto iter = clusterTimeMap_.find(uid);
                if (iter != clusterTimeMap_.end()) {
                    for (uint16_t i = 0; i < clusters.size(); i++) {
                        iter->second[i] += increments[i];
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Update cpu cluster time to: %{public}ld ms\
                            for uid: %{public}d", iter->second[i], uid);
                    }
                } else {
                    clusterTimeMap_.insert(std::pair<int32_t, std::vector<long>>(uid, increments));
                    STATS_HILOGD(STATS_MODULE_SERVICE, "Add cpu cluster time for uid: %{public}d", uid);
                }
            } else {
                STATS_HILOGD(STATS_MODULE_SERVICE, "Power supply is connected, don't add the increment");
            }
        }
        STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
        return true;
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "Opening file: %{public}s failed", UID_CPU_CLUSTER_TIME_FILE.c_str());
        return false;
    }
}

bool CpuTimeReader::ReadUidCpuFreqTime()
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    std::ifstream input(UID_CPU_FREQ_TIME_FILE);
    if (input) {
        std::string line;
        int32_t uid = -1;
        std::map<uint32_t, std::vector<long>> speedTime;
        while (getline(input, line)) {
            speedTime.clear();
            std::vector<std::string> splitedLine;
            split(line, ':', splitedLine);
            if (splitedLine[0] == "uid") {
                continue;
            } else {
                uid = stoi(splitedLine[0]);
            }

            std::vector<std::string> splitedTime;
            split(splitedLine[1], ' ', splitedTime);

            auto statsService = DelayedStatsSpSingleton<BatteryStatsService>::GetInstance();
            auto parser = statsService->GetBatteryStatsParser();
            uint16_t clusterNum = parser->GetClusterNum();
            uint16_t count = 0;
            for (uint16_t i = 0; i < clusterNum; i++) {
                std::vector<long> tempSpeedTimes;
                tempSpeedTimes.clear();
                for (int j = 0; j < parser->GetSpeedNum(i); j++) {
                    long tempTimeMs = stol(splitedTime[count++]) * 10; // Unit is 10ms
                    tempSpeedTimes.push_back(tempTimeMs);
                    STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu freq time: %{public}ld", tempTimeMs);
                }
                speedTime.insert(std::pair<uint32_t, std::vector<long>>(i, tempSpeedTimes));
            }

            std::map<uint32_t, std::vector<long>> increments;
            auto iterLast = lastFreqTimeMap_.find(uid);
            if (iterLast != lastFreqTimeMap_.end()) {
                for (uint16_t i = 0; i < clustersMap_.size(); i++) {
                    auto iterLastTemp = iterLast->second.find(i);
                    if (iterLastTemp != iterLast->second.end()) {
                        std::vector<long> lastSpeedTimes = iterLastTemp->second;
                        std::vector<long> newIncrementTimes;
                        newIncrementTimes.clear();
                        for (uint16_t j = 0; j < lastSpeedTimes.size(); j++) {
                            long increment = speedTime.at(i)[j] - lastSpeedTimes[j];
                            if (increment > 0) {
                                newIncrementTimes.push_back(increment);
                                increments.insert(std::pair<uint32_t, std::vector<long>>(i, newIncrementTimes));
                                STATS_HILOGD(STATS_MODULE_SERVICE,
                                    "Got cpu freq time increment: %{public}ld ms, uid: %{public}d", increment, uid);
                            } else {
                                STATS_HILOGE(STATS_MODULE_SERVICE, "Negative cpu freq time increment got");
                                return false;
                            }
                        }
                        iterLastTemp->second = speedTime.at(i);
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Update last cpu freq time for uid: %{public}d", uid);
                    }
                }
            } else {
                lastFreqTimeMap_.insert(std::pair<int32_t, std::map<uint32_t, std::vector<long>>>(uid, speedTime));
                increments = speedTime;
                STATS_HILOGD(STATS_MODULE_SERVICE, "Add last cpu freq time for uid: %{public}d", uid);
            }

            std::map<uint32_t, std::vector<long>> uidIncrements = increments;
            if (wakelockCounts_ > 0) {
                for (uint16_t i = 0; i < clustersMap_.size(); i++) {
                    for (int j = 0; j < clustersMap_[i]; j++) {
                        uidIncrements.at(i)[j] = increments.at(i)[j] / 2;
                    }
                }
                // TO-DO, distribute half of cpu freq time to wakelock holders
            }

            if (TimeHelper::IsOnBattery() || BatteryStatsUtils::DEBUG) { // Just for debug
                auto iter = freqTimeMap_.find(uid);
                if (iter != freqTimeMap_.end()) {
                    for (uint16_t i = 0; i < clustersMap_.size(); i++) {
                        for (int j = 0; j < clustersMap_[i]; j++) {
                            iter->second.at(i)[j] += uidIncrements.at(i)[j];
                            STATS_HILOGD(STATS_MODULE_SERVICE, "Update cpu freq time to: %{public}ld ms\
                                for uid: %{public}d", iter->second.at(i)[j], uid);
                        }
                    }
                } else {
                    freqTimeMap_.insert(std::pair<int32_t, std::map<uint32_t, std::vector<long>>>(uid, increments));
                    STATS_HILOGD(STATS_MODULE_SERVICE, "Add cpu freq time for uid: %{public}d", uid);
                }
            } else {
                STATS_HILOGD(STATS_MODULE_SERVICE, "Power supply is connected, don't add the increment");
            }
        }
        STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
        return true;
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "Opening file: %{public}s failed", UID_CPU_CLUSTER_TIME_FILE.c_str());
        return false;
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
}

bool CpuTimeReader::ReadUidCpuTime()
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    std::ifstream input(UID_CPU_TIME_FILE);
    if (input) {
        std::string line;
        int32_t uid = -1;
        std::vector<long> cpuTime;
        while (getline(input, line)) {
            cpuTime.clear();
            std::vector<std::string> splitedLine;
            split(line, ':', splitedLine);
            uid = stoi(splitedLine[0]);

            std::vector<std::string> splitedTime;
            split(splitedLine[1], ' ', splitedTime);
            for (uint16_t i = 0; i < splitedTime.size(); i++) {
                long tempTime = 0;
                tempTime = stol(splitedTime[i]);
                cpuTime.push_back(tempTime);
                STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu time: %{public}ld", tempTime);
            }

            std::vector<long> increments;
            auto iterLast = lastUidTimeMap_.find(uid);
            if (iterLast != lastUidTimeMap_.end()) {
                for (uint16_t i = 0; i < splitedTime.size(); i++) {
                    long increment = 0;
                    increment = cpuTime[i] - iterLast->second[i];
                    if (increment > 0) {
                        iterLast->second[i] = cpuTime[i];
                        increments.push_back(increment);
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Got cpu time increment: %{public}ld ms, uid: %{public}d",
                            increment, uid);
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Update last cpu time to: %{public}ld ms, uid: %{public}d",
                            cpuTime[i], uid);
                    } else {
                        STATS_HILOGE(STATS_MODULE_SERVICE, "Negative cpu time increment got");
                        return false;
                    }
                }
            } else {
                lastUidTimeMap_.insert(std::pair<int32_t, std::vector<long>>(uid, cpuTime));
                increments = cpuTime;
                STATS_HILOGD(STATS_MODULE_SERVICE, "Add last cpu time for uid: %{public}d", uid);
            }

            std::vector<long> uidIncrements = increments;
            if (wakelockCounts_ > 0) {
                uidIncrements[0] = increments[0] / 1000 / 2;
                uidIncrements[1] = increments[1] / 1000 / 2;
                // TO-DO, distribute half of cpu time to wakelock holders
            }

            if (TimeHelper::IsOnBattery() || BatteryStatsUtils::DEBUG) { // Just for debug
                auto iter = uidTimeMap_.find(uid);
                if (iter != uidTimeMap_.end()) {
                    for (uint16_t i = 0; i < uidIncrements.size(); i++) {
                        iter->second[i] = uidIncrements[i];
                        STATS_HILOGD(STATS_MODULE_SERVICE, "Update cpu time to: %{public}ld ms for uid: %{public}d",
                            iter->second[i], uid);
                    }
                } else {
                    uidTimeMap_.insert(std::pair<int32_t, std::vector<long>>(uid, uidIncrements));
                    STATS_HILOGD(STATS_MODULE_SERVICE, "Add cpu time for uid: %{public}d", uid);
                }
            } else {
                STATS_HILOGD(STATS_MODULE_SERVICE, "Power supply is connected, don't add the increment");
            }
        }
        STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
        return true;
    } else {
        STATS_HILOGE(STATS_MODULE_SERVICE, "Opening file: %{public}s failed", UID_CPU_CLUSTER_TIME_FILE.c_str());
        return false;
    }
}

void CpuTimeReader::split(std::string &origin, char delimiter, std::vector<std::string> &splited)
{
    STATS_HILOGI(STATS_MODULE_SERVICE, "Enter");
    size_t start;
    size_t end = 0;

    while ((start = origin.find_first_not_of(delimiter, end)) != std::string::npos) {
        end = origin.find(delimiter, start);
        splited.push_back(origin.substr(start, end - start));
    }
    STATS_HILOGI(STATS_MODULE_SERVICE, "Exit");
}
} // namespace PowerMgr
} // namespace OHOS