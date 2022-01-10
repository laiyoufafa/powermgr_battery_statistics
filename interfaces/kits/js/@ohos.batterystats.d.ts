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

import { AsyncCallback } from "./basic";
/**
 * Obtains power consumption information list of a device.
 *
 * <p>A array of power consumption information list.
 *
 * @SysCap SystemCapability.PowerMgr.BatteryStatistics
 * @devices phone, tablet
 * @since 6
 */
declare namespace batteryStats {

  export enum StatsType {
    /**
     * Indicates an invalid battery statistics type
     */
    STATS_TYPE_INVALID = -10,
    /**
     * Indicates the battery power consumption generated by APP
     */
    STATS_TYPE_APP,
    /**
     * Indicates the battery power consumption generated by bluetooth
     */
    STATS_TYPE_BLUETOOTH,
    /**
     * Indicates the battery power consumption generated when the CPU is idle
     */
    STATS_TYPE_IDLE,
    /**
     * Indicates the battery power consumption generated when phone call is active
     */
    STATS_TYPE_PHONE,
    /**
     * Indicates the battery power consumption generated by radio
     */
    STATS_TYPE_RADIO,
    /**
     * Indicates the battery power consumption generated by screen
     */
    STATS_TYPE_SCREEN,
    /**
     * Indicates the battery power consumption generated by user
     */
    STATS_TYPE_USER,
    /**
     * Indicates the battery power consumption generated by WIFI
     */
    STATS_TYPE_WIFI
  }
  /**
   * Obtains power consumption information list of a device.
   *
   * @devices phone, tablet
   * @since 3
   */
  function getBatteryStats(): Promise<Array<BatteryStatsInfo>>;
  function getBatteryStats(callback: AsyncCallback<Array<BatteryStatsInfo>>): void;
  /**
   * Obtains power consumption information(Mah) for a given uid.
   *
   * @devices phone, tablet
   * @since 3
   */
  function getAppPowerValue(uid: number): number;
  /**
   * Obtains power consumption information(Percent) for a given uid.
   *
   * @devices phone, tablet
   * @since 3
   */
  function getAppPowerPercent(uid: number): number;
  /**
   * Obtains power consumption information(Mah) for a given type.
   *
   * @devices phone, tablet
   * @since 3
   */
  function getHardwareUnitPowerValue(type: StatsType): number;
  /**
   * Obtains power consumption information(Percent) for a given type.
   *
   * @devices phone, tablet
   * @since 3
   */
  function getHardwareUnitPowerPercent(type: StatsType): number;
  /**
   * Contains power consumption information of a device.
   *
   * <p>Power consumption information includes the uid, type and
   * power consumption value.
   *
   * @SysCap SystemCapability.PowerMgr.BatteryStatistics
   * @devices phone, tablet
   * @since 6
   */
  interface BatteryStatsInfo {
    /**
     * The uid related with the power consumption info.
     * @devices phone, tablet
     */
    uid: number;

    /**
      * The type related with the power consumption info.
      * @devices phone, tablet
      */
    type: StatsType;

    /**
      * The power consumption value(mah).
      * @devices phone, tablet
      */
    power: number;
  }
}
export default batteryStats;