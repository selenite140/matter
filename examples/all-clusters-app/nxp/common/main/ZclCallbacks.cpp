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

#include <lib/support/logging/CHIPLogging.h>

#include "AppTask.h"
#include "ClusterManager.h"
#include "CHIPDeviceManager.h"

#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app/ConcreteAttributePath.h>
#include <app/util/af-types.h>
#include <app/util/af.h>

using namespace ::chip;
using namespace ::chip::app::Clusters;

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath & path, uint8_t mask, uint8_t type, uint16_t size,
                                       uint8_t * value)
{
    chip::DeviceManager::CHIPDeviceManagerCallbacks * cb =
        chip::DeviceManager::CHIPDeviceManager::GetInstance().GetCHIPDeviceManagerCallbacks();
    if (cb != nullptr)
    {
        // propagate event to device manager
        cb->PostAttributeChangeCallback(path.mEndpointId, path.mClusterId, path.mAttributeId, mask, type, size, value);
    }

    if (path.mClusterId == OnOff::Id)
    {
        if (path.mAttributeId != OnOff::Attributes::OnOff::Id)
        {
            ChipLogProgress(Zcl, "Unknown attribute ID: " ChipLogFormatMEI, ChipLogValueMEI(path.mAttributeId));
            return;
        }

        ClusterMgr().InitiateAction(0, *value ? ClusterManager::TURNON_ACTION : ClusterManager::TURNOFF_ACTION);
    }
    else if (path.mClusterId == LevelControl::Id)
    {
        ChipLogProgress(Zcl,
                        "Level Control attribute ID: " ChipLogFormatMEI " Type: %" PRIu8 " Value: %" PRIu16 ", length %" PRIu16,
                        ChipLogValueMEI(path.mAttributeId), type, *value, size);

        // WIP Apply attribute change to Light
    }
    else if (path.mClusterId == ColorControl::Id)
    {
        ChipLogProgress(Zcl,
                        "Color Control attribute ID: " ChipLogFormatMEI " Type: %" PRIu8 " Value: %" PRIu16 ", length %" PRIu16,
                        ChipLogValueMEI(path.mAttributeId), type, *value, size);

        // WIP Apply attribute change to Light
    }
    else if (path.mClusterId == OnOffSwitchConfiguration::Id)
    {
        ChipLogProgress(Zcl,
                        "OnOff Switch Configuration attribute ID: " ChipLogFormatMEI " Type: %" PRIu8 " Value: %" PRIu16
                        ", length %" PRIu16,
                        ChipLogValueMEI(path.mAttributeId), type, *value, size);

        // WIP Apply attribute change to Light
    }
    else
    {
        ChipLogProgress(Zcl, "Unknown attribute ID: " ChipLogFormatMEI, ChipLogValueMEI(path.mAttributeId));
    }
}

/** @brief OnOff Cluster Init
 *
 * This function is called when a specific cluster is initialized. It gives the
 * application an opportunity to take care of cluster initialization procedures.
 * It is called exactly once for each endpoint where cluster is present.
 *
 * @param endpoint   Ver.: always
 *
 * TODO Issue #3841
 * emberAfOnOffClusterInitCallback happens before the stack initialize the cluster
 * attributes to the default value.
 * The logic here expects something similar to the deprecated Plugins callback
 * emberAfPluginOnOffClusterServerPostInitCallback.
 *
 */


void emberAfOnOffClusterInitCallback(EndpointId endpoint)
{
    GetAppTask().UpdateClusterState();
}
