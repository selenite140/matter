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
#include <lib/core/CHIPError.h>
#include "AppEvent.h"

#include "FreeRTOS.h"
#include "timers.h" // provides FreeRTOS timer support

class ClusterManager
{
public:
    enum Action_t
    {
        TURNON_ACTION = 0,
        TURNOFF_ACTION,

        INVALID_ACTION
    } Action;

    enum State_t
    {
        kState_TurnOnInitiated = 0,
        kState_TurnOnCompleted,
        kState_TurnOffInitiated,
        kState_TurnOffCompleted,
    } State;

    CHIP_ERROR Init();
    bool IsTurnedOff();
    void EnableAutoTurnOn(bool aOn);
    void SetAutoTurnOnDuration(uint32_t aDurationInSecs);
    bool IsActionInProgress();
    bool InitiateAction(int32_t aActor, Action_t aAction);

    typedef void (*Callback_fn_initiated)(Action_t, int32_t aActor);
    typedef void (*Callback_fn_completed)(Action_t);
    void SetCallbacks(Callback_fn_initiated aActionInitiated_CB, Callback_fn_completed aActionCompleted_CB);

private:
    friend ClusterManager & ClusterMgr(void);
    State_t mState;

    Callback_fn_initiated mActionInitiated_CB;
    Callback_fn_completed mActionCompleted_CB;

    bool mAutoTurnOn;
    uint32_t mAutoTurnOnDuration;
    bool mAutoTurnOnTimerArmed;

    void CancelTimer(void);
    void StartTimer(uint32_t aTimeoutMs);

    static void TimerEventHandler(TimerHandle_t xTimer);
    static void AutoReTurnOnTimerEventHandler(AppEvent * aEvent);
    static void ActuatorMovementTimerEventHandler(AppEvent * aEvent);

    static ClusterManager sCluster;
};

inline ClusterManager & ClusterMgr(void)
{
    return ClusterManager::sCluster;
}