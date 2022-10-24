/*
 *
 *    Copyright (c) 2021-2022 Project CHIP Authors
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
#include "AppTask.h"
#include "AppEvent.h"
//#include "LEDWidget.h"
#include "lib/support/ErrorStr.h"
#include <app/server/Server.h>
#include <app/server/Dnssd.h>
#include <lib/dnssd/Advertiser.h>

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
#include <platform/nxp/common/NetworkCommissioningDriver.h>
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/OnboardingCodesUtil.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/internal/DeviceNetworkInfo.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <lib/support/ThreadOperationalDataset.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/util/attribute-storage.h>

#include "board.h"
#include "fsl_component_timer_manager.h"
#include "fsl_clock.h"
#include "application_config.h"
#include "board_comp.h"
#include "fwk_platform.h"

#include "MatterCli.h"

#if CHIP_DEVICE_CONFIG_ENABLE_OTA_REQUESTOR
#include "OTARequestorInitiator.h"
#endif

#if CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA
#include "FactoryDataProvider.h"
#endif

#if CHIP_ENABLE_OPENTHREAD
#include <inet/EndPointStateOpenThread.h>
#endif

#define FACTORY_RESET_TRIGGER_TIMEOUT 6000
#define FACTORY_RESET_CANCEL_WINDOW_TIMEOUT 3000
#define APP_TASK_STACK_SIZE (4096)
#define APP_TASK_PRIORITY 2
#define APP_EVENT_QUEUE_SIZE 10

TimerHandle_t sFunctionTimer; // FreeRTOS app sw timer.

static QueueHandle_t sAppEventQueue;

using namespace chip;
using namespace chip::TLV;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;
using namespace ::chip::app::Clusters;

#if CHIP_DEVICE_CONFIG_ENABLE_OTA_REQUESTOR
OTARequestorInitiator gOTARequestorInitiator;
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
namespace {
chip::app::Clusters::NetworkCommissioning::Instance
    sWiFiNetworkCommissioningInstance(0 /* Endpoint Id */, &(::chip::DeviceLayer::NetworkCommissioning::NXPWiFiDriver::GetInstance()));
} // namespace

void NetWorkCommissioningInstInit()
{
    sWiFiNetworkCommissioningInstance.Init();
}
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::StartAppTask()
{
    CHIP_ERROR err = CHIP_NO_ERROR;
    TaskHandle_t taskHandle;

    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL)
    {
        err = CHIP_ERROR_NO_MEMORY;
        ChipLogError(DeviceLayer, "Failed to allocate app event queue");
        assert(err == CHIP_NO_ERROR);
    }

    if (xTaskCreate(&AppTask::AppTaskMain, "AppTaskMain", 1600,
                    &sAppTask, APP_TASK_PRIORITY, &taskHandle) != pdPASS)
    {
        err = CHIP_ERROR_NO_MEMORY;
        ChipLogError(DeviceLayer, "Failed to start app task");
        assert(err == CHIP_NO_ERROR);
    }

    return err;
}

#if CHIP_ENABLE_OPENTHREAD
void LockOpenThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().LockThreadStack();
}

void UnlockOpenThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().UnlockThreadStack();
}
#endif

void AppTask::InitServer(intptr_t arg)
{
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();

#if CHIP_ENABLE_OPENTHREAD
    // Init ZCL Data Model and start server
    chip::Inet::EndPointStateOpenThread::OpenThreadEndpointInitParam nativeParams;
    nativeParams.lockCb                = LockOpenThreadTask;
    nativeParams.unlockCb              = UnlockOpenThreadTask;
    nativeParams.openThreadInstancePtr = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
    initParams.endpointNativeParams    = static_cast<void *>(&nativeParams);
#endif
    VerifyOrDie((chip::Server::GetInstance().Init(initParams)) == CHIP_NO_ERROR);

    // Disable last fixed endpoint, which is used as a placeholder for all of the
    // supported clusters so that ZAP will generated the requisite code.
    emberAfEndpointEnableDisable(emberAfEndpointFromIndex(static_cast<uint16_t>(emberAfFixedEndpointCount() - 1)), false);

#if ENABLE_OTA_PROVIDER
    InitOTAServer();
#endif

}

CHIP_ERROR AppTask::Init()
{
    //chip::app::Dnssd::StartServer();
    //chip::app::Dnssd::AdvertiseCommissionableNode();
    
    //chip::app::Dnssd::ServiceAdvertiser::Instance().Init();
    
    CHIP_ERROR err = CHIP_NO_ERROR;
    button_config_t buttonConfig;
    button_status_t bStatus;
    timer_status_t tStatus; 

    // Init ZCL Data Model and start server
    PlatformMgr().ScheduleWork(InitServer, 0);

#if CONFIG_CHIP_PLAT_LOAD_REAL_FACTORY_DATA
    FactoryDataProvider *factoryDataProvider = &FactoryDataProvider::GetDefaultInstance();
    SetDeviceInstanceInfoProvider(factoryDataProvider);
    SetDeviceAttestationCredentialsProvider(factoryDataProvider);
    SetCommissionableDataProvider(factoryDataProvider);
#else
    // Initialize device attestation when the example (only for debug purpose)
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
    NetWorkCommissioningInstInit();
#endif // CHIP_DEVICE_CONFIG_ENABLE_WPA

    uint16_t discriminator;
    err = GetCommissionableDataProvider()->GetSetupDiscriminator(discriminator);
    if (err != CHIP_NO_ERROR)
    {
        PRINTF("\r\nCouldn't get discriminator: %s", chip::ErrorStr(err));
    }
    PRINTF("\r\nSetup discriminator: %u (0x%x)", discriminator, discriminator);

    uint32_t setupPasscode=20202021;
    err = GetCommissionableDataProvider()->GetSetupPasscode(setupPasscode);
    if (err != CHIP_NO_ERROR)
    {
        PRINTF("\r\nCouldn't get setupPasscode: %s", chip::ErrorStr(err));
    }
    PRINTF("\r\nSetup passcode: %u (0x%x)", setupPasscode, setupPasscode);

    uint16_t vendorId;
    err = GetDeviceInstanceInfoProvider()->GetVendorId(vendorId);
    if (err != CHIP_NO_ERROR)
    {
        PRINTF("\r\nCouldn't get vendorId: %s", chip::ErrorStr(err));
    }
    PRINTF("\r\nVendor ID: %u (0x%x)", vendorId, vendorId);


    uint16_t productId;
    err = GetDeviceInstanceInfoProvider()->GetProductId(productId);
    if (err != CHIP_NO_ERROR)
    {
        PRINTF("\r\nCouldn't get productId: %s", chip::ErrorStr(err));
    }
    PRINTF("\r\nProduct ID: %u (0x%x)\r\n", productId, productId);
     
    // QR code will be used with CHIP Tool
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kOnNetwork));

    /* Init the SDK timer manager */
    tStatus = PLATFORM_InitTimerManager();
    if (tStatus != kStatus_TimerSuccess)
    {
        err = CHIP_ERROR_UNEXPECTED_EVENT;
        ChipLogError(DeviceLayer, "tmr init error");       
    }

    /* Init the SDK Button component */
    bStatus = BOARD_InitButton((button_handle_t)sdkButtonHandle);
    if (bStatus != kStatus_BUTTON_Success)
    {
        err = CHIP_ERROR_UNEXPECTED_EVENT;
        ChipLogError(DeviceLayer, "button init error");       
    }
    bStatus = BUTTON_InstallCallback((button_handle_t)sdkButtonHandle, AppTask::SdkButtonCallback, this);

    if (bStatus != kStatus_BUTTON_Success)
    {
        err = CHIP_ERROR_UNEXPECTED_EVENT;
        ChipLogError(DeviceLayer, "button init error");       
    }

    // Create FreeRTOS sw timer for Function Selection.
    sFunctionTimer = xTimerCreate("FnTmr",          // Just a text name, not used by the RTOS kernel
                                  1,                // == default timer period (mS)
                                  false,            // no timer reload (==one-shot)
                                  (void *) this,    // init timer id = app task obj context
                                  TimerEventHandler // timer callback handler
    );
    if (sFunctionTimer == NULL)
    {
        err = CHIP_ERROR_UNEXPECTED_EVENT;
        ChipLogError(DeviceLayer, "app_timer_create() failed");
        assert(err == CHIP_NO_ERROR);
    }

    err = ClusterMgr().Init();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "ClusterMgr().Init() failed");
        assert(err == CHIP_NO_ERROR);
    }

    ClusterMgr().SetCallbacks(ActionInitiated, ActionCompleted);

    // Print the current software version
    char currentSoftwareVer[ConfigurationManager::kMaxSoftwareVersionStringLength + 1] = { 0 };
    err = ConfigurationMgr().GetSoftwareVersionString(currentSoftwareVer, sizeof(currentSoftwareVer));
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "Get version error");
        assert(err == CHIP_NO_ERROR);
    }

    ChipLogProgress(DeviceLayer, "Current Software Version: %s", currentSoftwareVer);

#if CHIP_DEVICE_CONFIG_ENABLE_OTA_REQUESTOR 
    /* If an update is under test make it permanent */
    gOTARequestorInitiator.HandleSelfTest();
    /* Initialize OTA Requestor */
    PlatformMgr().ScheduleWork(gOTARequestorInitiator.InitOTA, reinterpret_cast<intptr_t>(&gOTARequestorInitiator));
#endif

    addMatterCliCommands();

    return err;
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppTask *task = (AppTask *)pvParameter;
    CHIP_ERROR err;
    AppEvent event;

    err = task->Init();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "AppTask.Init() failed");
        assert(err == CHIP_NO_ERROR);
    }

    while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, portMAX_DELAY);
        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }
    }
}

button_status_t AppTask::SdkButtonCallback(void *buttonHandle, button_callback_message_t *message, void *callbackParam)
{
    switch (message->event)
    {
        case kBUTTON_EventShortPress:
            static_cast<AppTask *>(callbackParam)->ButtonEventHandler(TCPIP_BUTTON, TCPIP_BUTTON_PUSH);
            break;
        case kBUTTON_EventLongPress:
            static_cast<AppTask *>(callbackParam)->ButtonEventHandler(TCPIP_BUTTON, RESET_BUTTON_PUSH);
            break;
        default:
            break;
    }
    return kStatus_BUTTON_Success;
}

void AppTask::ButtonEventHandler(uint8_t pin_no, uint8_t button_action)
{
    if (pin_no != TCPIP_BUTTON)
    {
        return;
    }

    AppEvent button_event;
    button_event.Type               = AppEvent::kEventType_Button;
    button_event.ButtonEvent.PinNo  = pin_no;
    button_event.ButtonEvent.Action = button_action;

    if (pin_no == TCPIP_BUTTON)
    {
        button_event.Handler = BleHandler;
        if (button_action == RESET_BUTTON_PUSH)
        {
            button_event.Handler = ResetActionEventHandler;
        }
    }

    sAppTask.PostEvent(&button_event);
}


void AppTask::TimerEventHandler(TimerHandle_t xTimer)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Timer;
    event.TimerEvent.Context = (void *) xTimer;
    event.Handler            = FunctionTimerEventHandler;
    sAppTask.PostEvent(&event);
}

void AppTask::FunctionTimerEventHandler(AppEvent * aEvent)
{
    if (aEvent->Type != AppEvent::kEventType_Timer)
        return;

    ChipLogProgress(DeviceLayer, "Device will factory reset...");

    // Actually trigger Factory Reset
    ConfigurationMgr().InitiateFactoryReset();
}

void AppTask::ResetActionEventHandler(AppEvent * aEvent)
{
    if (aEvent->ButtonEvent.PinNo != RESET_BUTTON && aEvent->ButtonEvent.PinNo != TCPIP_BUTTON)
        return;

    if (sAppTask.mFunction != kFunction_NoneSelected && sAppTask.mFunction != kFunction_FactoryReset)
    {
        ChipLogProgress(DeviceLayer, "Another function is scheduled. Could not toggle Factory Reset state!");
        return;
    }

    if (sAppTask.mResetTimerActive)
    {
        sAppTask.CancelTimer();
        sAppTask.mFunction = kFunction_NoneSelected;

        ChipLogProgress(DeviceLayer, "Factory Reset was cancelled!");
    }
    else
    {
        uint32_t resetTimeout = FACTORY_RESET_TRIGGER_TIMEOUT;

        if (sAppTask.mFunction != kFunction_NoneSelected)
        {
            ChipLogProgress(DeviceLayer, "Another function is scheduled. Could not initiate Factory Reset!");
            return;
        }

        ChipLogProgress(DeviceLayer, "Factory Reset Triggered. Push the RESET button within %lu ms to cancel!", resetTimeout);
        sAppTask.mFunction = kFunction_FactoryReset;
        sAppTask.StartTimer(FACTORY_RESET_TRIGGER_TIMEOUT);
    }
}

void AppTask::BleHandler(AppEvent * aEvent)
{
    if (aEvent->ButtonEvent.PinNo != TCPIP_BUTTON)
        return;

    if (sAppTask.mFunction != kFunction_NoneSelected)
    {
        ChipLogProgress(DeviceLayer, "Another function is scheduled. Could not toggle BLE state!");
        return;
    }

    if (ConnectivityMgr().IsBLEAdvertisingEnabled())
    {
        ConnectivityMgr().SetBLEAdvertisingEnabled(false);
        ChipLogProgress(DeviceLayer, "Stopped BLE Advertising!");
    }
    else
    {
        ConnectivityMgr().SetBLEAdvertisingEnabled(true);

        if (chip::Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() == CHIP_NO_ERROR)
        {
            ChipLogProgress(DeviceLayer, "Started BLE Advertising!");
        }
        else
        {
            ChipLogProgress(DeviceLayer, "OpenBasicCommissioningWindow() failed");
        }
    }
}

void AppTask::CancelTimer()
{
    if (xTimerStop(sFunctionTimer, 0) == pdFAIL)
    {
        ChipLogError(DeviceLayer, "app timer stop() failed");
    }

    mResetTimerActive = false;
}

void AppTask::StartTimer(uint32_t aTimeoutInMs)
{
    if (xTimerIsTimerActive(sFunctionTimer))
    {
        ChipLogError(DeviceLayer, "app timer already started!");
        CancelTimer();
    }

    // timer is not active, change its period to required value (== restart).
    // FreeRTOS- Block for a maximum of 100 ticks if the change period command
    // cannot immediately be sent to the timer command queue.
    if (xTimerChangePeriod(sFunctionTimer, aTimeoutInMs / portTICK_PERIOD_MS, 100) != pdPASS)
    {
        ChipLogError(DeviceLayer, "app timer start() failed");
    }

    mResetTimerActive = true;
}

void AppTask::ActionInitiated(ClusterManager::Action_t aAction, int32_t aActor)
{
    // start flashing the LEDs rapidly to indicate action initiation.
    if (aAction == ClusterManager::TURNON_ACTION)
    {
        ChipLogProgress(DeviceLayer, "Turn on Action has been initiated");
    }
    else if (aAction == ClusterManager::TURNOFF_ACTION)
    {
        ChipLogProgress(DeviceLayer, "Turn off Action has been initiated");
    }

    sAppTask.mFunction = kFunctionTurnOnTurnOff;
}

void AppTask::ActionCompleted(ClusterManager::Action_t aAction)
{
   
    if (aAction == ClusterManager::TURNON_ACTION)
    {
        ChipLogProgress(DeviceLayer, "Turn on action has been completed");
    }
    else if (aAction == ClusterManager::TURNOFF_ACTION)
    {
        ChipLogProgress(DeviceLayer, "Turn off action has been completed");
    }

    sAppTask.mFunction = kFunction_NoneSelected;
}

void AppTask::PostEvent(const AppEvent * aEvent)
{
    if (sAppEventQueue != NULL)
    {
        if (!xQueueSend(sAppEventQueue, aEvent, 1))
        {
            ChipLogError(DeviceLayer, "Failed to post event to app task event queue");
        }
    }
}

void AppTask::DispatchEvent(AppEvent * aEvent)
{
    if (aEvent->Handler)
    {
        aEvent->Handler(aEvent);
    }
    else
    {
        ChipLogProgress(DeviceLayer, "Event received with no handler. Dropping event.");
    }
}

void AppTask::UpdateClusterState(void)
{
    uint8_t newValue = 0;//!ClusterMgr().IsTurnedOff();

    // write the new on/off value
    EmberAfStatus status = emberAfWriteAttribute(1, ZCL_ON_OFF_CLUSTER_ID, ZCL_ON_OFF_ATTRIBUTE_ID,
                                                 (uint8_t *) &newValue, ZCL_BOOLEAN_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        ChipLogError(NotSpecified, "ERR: updating on/off %x", status);
    }
}
