/*
 *  Copyright 2022 NXP
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#include "MatterCli.h"
#include <cstring>
#include <platform/CHIPDeviceLayer.h>
#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
#include <openthread/cli.h>
#endif

void cliBLEAdvertising(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    if (aArgsLength == 0)
    {
        if (chip::DeviceLayer::ConnectivityMgr().IsBLEAdvertisingEnabled())
        {
            chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(false);
        }
        else
        {
            chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
        }
    }
    else if (strncmp(aArgs[0], "on", 2) == 0)
    {
        chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(true);
    }
    else if (strncmp(aArgs[0], "off", 3) == 0)
    {
        chip::DeviceLayer::ConnectivityMgr().SetBLEAdvertisingEnabled(false);
    }
}

void cliFactoryReset(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    if (aArgsLength == 0)
    {
        // Trigger Factory Reset
        chip::DeviceLayer::ConfigurationMgr().InitiateFactoryReset();
    }
}

void addMatterCliCommands(){

#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
    const otCliCommand kCommands[] = {
        {"bleadvertising", cliBLEAdvertising},
        {"matterfactoryreset", cliFactoryReset},
    };

    otCliSetUserCommands(kCommands, sizeof(kCommands) / sizeof(kCommands[0]), NULL);
#endif

}
