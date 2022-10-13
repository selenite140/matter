/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

/* platform specific */
#include "board.h"
#include "clock_config.h"
#include "fsl_debug_console.h"
#include "fsl_gpio.h"
#include "pin_mux.h"

#include <wm_os.h>
extern "C" {
#include "boot_flags.h"
#include "cli.h"
#include "dhcp-server.h"
#include "iperf.h"
#include "mflash_drv.h"
#include "network_flash_storage.h"
#include "partition.h"
#include "ping.h"
#include "wlan.h"
#include "wm_net.h"
}
#include "fsl_aes.h"
#include "lpm.h"




//using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;
using namespace chip;

AppTask AppTask::sAppTask;


CHIP_ERROR AppTask::Init()
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    return err;
}

void led_on_off(led_id_t lt_id, bool is_on)
{
    GPIO_Type * pgpio;
    uint32_t gpio_pin;

    // Configure the GPIO / PIN
    switch (lt_id)
    {
    case led_amber:
        pgpio    = BOARD_LED_AMBER_GPIO;
        gpio_pin = BOARD_LED_AMBER_GPIO_PIN;
        break;
    case led_yellow:
    default: // Note: led_yellow as default
        pgpio    = BOARD_LED_YELLOW_GPIO;
        gpio_pin = BOARD_LED_YELLOW_GPIO_PIN;
    }
    // Do on/off the LED
    if (is_on == true)
    {
        // PRINTF("led on\r\n");
        GPIO_PortClear(pgpio, GPIO_PORT(gpio_pin), 1u << GPIO_PORT_PIN(gpio_pin));
    }
    else
    {
        // PRINTF("led off\r\n");
        GPIO_PortSet(pgpio, GPIO_PORT(gpio_pin), 1u << GPIO_PORT_PIN(gpio_pin));
    }
    return;
}

