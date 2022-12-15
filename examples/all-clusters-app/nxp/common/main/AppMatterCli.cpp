/*
 *
 *    Copyright (c) 2022 Project CHIP Authors
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

#include "AppMatterCli.h"
#include <cstring>
#include <platform/CHIPDeviceLayer.h>
#include "AppTask.h"
#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
#include <openthread/cli.h>
#define MATTER_CLI_LOG otCliOutputFormat
#else
#define MATTER_CLI_LOG(...)
#endif

#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
static otError commissioningManager(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    otError error = OT_ERROR_NONE;
    if (strncmp(aArgs[0], "on", 2) == 0)
    {
        GetAppTask().StartCommissioningHandler();
    }
    else if (strncmp(aArgs[0], "off", 3) == 0)
    {
        GetAppTask().StopCommissioningHandler();
    }
    else
    {
        MATTER_CLI_LOG("wrong args should be either \"mattercommissioning on\" or \"mattercommissioning off\"" );
        error = OT_ERROR_INVALID_ARGS;
    }
    return error;
}

static otError cliFactoryReset(void *aContext, uint8_t aArgsLength, char *aArgs[])
{
    GetAppTask().FactoryResetHandler();
    return OT_ERROR_NONE;
}
#endif

CHIP_ERROR AppMatterCli_RegisterCommands(void)
{
    /*
    * TODO change it to support the Matter CLI instead only relying on the ot_cli
    */
#if CHIP_ENABLE_OPENTHREAD && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
    static const otCliCommand kCommands[] = {
        {"mattercommissioning", commissioningManager},
        {"matterfactoryreset", cliFactoryReset},
    };

    otCliSetUserCommands(kCommands, sizeof(kCommands) / sizeof(kCommands[0]), NULL);
#endif
    return CHIP_NO_ERROR;
}
