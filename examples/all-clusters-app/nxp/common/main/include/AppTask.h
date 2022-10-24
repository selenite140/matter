/*
 *
 *    Copyright (c) 2021 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "AppEvent.h"
#include "ClusterManager.h"

#include <platform/CHIPDeviceLayer.h>

#include "fsl_component_button.h"

#include "FreeRTOS.h"
#include "timers.h"

#if ENABLE_OTA_PROVIDER
#include <OTAProvider.h>
#endif

class AppTask
{
public:
    CHIP_ERROR StartAppTask();
    static void AppTaskMain(void * pvParameter);

    void PostTurnOnActionRequest(int32_t aActor, ClusterManager::Action_t aAction);
    void PostEvent(const AppEvent * event);

    void UpdateClusterState(void);

private:
    friend AppTask & GetAppTask(void);

    CHIP_ERROR Init();

    static void ActionInitiated(ClusterManager::Action_t aAction, int32_t aActor);
    static void ActionCompleted(ClusterManager::Action_t aAction);

    void CancelTimer(void);

    void DispatchEvent(AppEvent * event);

    static void FunctionTimerEventHandler(AppEvent * aEvent);
    static void ClusterActionEventHandler(AppEvent * aEvent);
    static void ResetActionEventHandler(AppEvent * aEvent);
    static void BleHandler(AppEvent * aEvent);
    static void InstallEventHandler(AppEvent * aEvent);

    static void ButtonEventHandler(uint8_t pin_no, uint8_t button_action);
    static void TimerEventHandler(TimerHandle_t xTimer);

    static void ThreadProvisioningHandler(const chip::DeviceLayer::ChipDeviceEvent * event, intptr_t arg);

    void StartTimer(uint32_t aTimeoutInMs);

    static void InitServer(intptr_t arg);

    enum Function_t
    {
        kFunction_NoneSelected   = 0,
        kFunction_SoftwareUpdate = 0,
        kFunction_FactoryReset,
        kFunctionTurnOnTurnOff,

        kFunction_Invalid
    } Function;

    Function_t mFunction;
    bool mResetTimerActive;
    BUTTON_HANDLE_DEFINE(sdkButtonHandle);
    static button_status_t SdkButtonCallback(void *buttonHandle, button_callback_message_t *message, void *callbackParam);

    static AppTask sAppTask;
};

inline AppTask & GetAppTask(void)
{
    return AppTask::sAppTask;
}
