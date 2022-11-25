/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/util/af-types.h>
#include <app/util/attribute-storage.h>
#include <app/util/attribute-table.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>

#include "AppTask.h"

static const char * TAG = "mw320";

using namespace ::chip;
using namespace ::chip::app::Clusters;

/*
    Cluster: On/Off, Attribute Change Handler
*/
static void OnOnOffPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    VerifyOrExit(attributeId == ZCL_ON_OFF_ATTRIBUTE_ID,
                 ChipLogError(DeviceLayer, "Unhandled Attribute ID: '0x%04lx", attributeId));
    VerifyOrExit(((endpointId == 1) || (endpointId == 2)), ChipLogError(DeviceLayer, "Unexpected EndPoint ID: `0x%02x'", endpointId));

    // At this point we can assume that value points to a bool value.
    led_on_off(led_yellow, (*value != 0) ? true : false);

exit:
    return;
}


/*
    Cluster: Identify, Attribute Change Handler
*/
uint32_t identifyTimerCount;
constexpr uint32_t kIdentifyTimerDelayMS = 250;
typedef struct _Identify_Timer
{
    EndpointId ep;
    uint32_t identifyTimerCount;
} Identify_Time_t;
Identify_Time_t id_time[MAX_ENDPOINT_COUNT];

void IdentifyTimerHandler(System::Layer * systemLayer, void * appState)
{
    Identify_Time_t * pidt = (Identify_Time_t *) appState;
    ChipLogDetail(DeviceLayer, " -> %s(%u, %u) \r\n", __FUNCTION__, pidt->ep, pidt->identifyTimerCount);
    if (pidt->identifyTimerCount)
    {
        pidt->identifyTimerCount--;
        emAfWriteAttribute(pidt->ep, ZCL_IDENTIFY_CLUSTER_ID, ZCL_IDENTIFY_TIME_ATTRIBUTE_ID, (uint8_t *) &pidt->identifyTimerCount,
                           sizeof(identifyTimerCount), true, false);
        DeviceLayer::SystemLayer().StartTimer(System::Clock::Seconds16(1), IdentifyTimerHandler, pidt);
    }
}

static void OnIdentifyPostAttributeChangeCallback(EndpointId endpointId, AttributeId attributeId, uint8_t * value)
{
    VerifyOrExit(attributeId == ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                 ChipLogError(DeviceLayer, "[%s] Unhandled Attribute ID: '0x%04lx", TAG, attributeId));
    VerifyOrExit((endpointId < MAX_ENDPOINT_COUNT),
                 ChipLogError(DeviceLayer, "[%s] EndPoint > max: [%u, %u]", TAG, endpointId, MAX_ENDPOINT_COUNT));
    if (id_time[endpointId].identifyTimerCount != *value)
    {
        id_time[endpointId].ep                 = endpointId;
        id_time[endpointId].identifyTimerCount = *value;
        ChipLogDetail(DeviceLayer, "-> Identify: %u \r\n", id_time[endpointId].identifyTimerCount);
        DeviceLayer::SystemLayer().StartTimer(System::Clock::Seconds16(1), IdentifyTimerHandler, &id_time[endpointId]);
    }

exit:
    return;
}


/*
        Callback to receive the cluster modification event
*/
void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & path, uint8_t type, uint16_t size, uint8_t * value)
{
    switch (path.mClusterId)
    {
    case ZCL_ON_OFF_CLUSTER_ID:
        OnOnOffPostAttributeChangeCallback(path.mEndpointId, path.mAttributeId, value);
        break;
    case ZCL_IDENTIFY_CLUSTER_ID:
        OnIdentifyPostAttributeChangeCallback(path.mEndpointId, path.mAttributeId, value);
        break;
    default:
        break;
    }
    return;
}

EmberAfStatus emberAfExternalAttributeWriteCallback(EndpointId endpoint, ClusterId clusterId,
                                                    const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer)
{
    return EMBER_ZCL_STATUS_SUCCESS;
}

EmberAfStatus emberAfExternalAttributeReadCallback(EndpointId endpoint, ClusterId clusterId,
                                                   const EmberAfAttributeMetadata * attributeMetadata, uint8_t * buffer,
                                                   uint16_t maxReadLength)
{
    return EMBER_ZCL_STATUS_SUCCESS;
}


