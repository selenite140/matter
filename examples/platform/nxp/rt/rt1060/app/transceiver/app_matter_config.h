/*
 *  Copyright 2020-2022 NXP
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 */

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE

#define USB_HOST_CONFIG_EHCI 2
#define CONTROLLER_ID        kUSB_ControllerEhci0

#if !(defined(WIFI_IW416_BOARD_AW_AM457_USD) || defined(WIFI_IW416_BOARD_AW_AM510_USD) || defined(WIFI_88W8987_BOARD_AW_CM358_USD) || defined(K32W061_TRANSCEIVER))
#error The transceiver module is unsupported
#endif

#endif /* CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE */

#if CHIP_DEVICE_CONFIG_ENABLE_WPA
#define NOT_DEFINE_DEFAULT_WIFI_MODULE
#include "app_config.h"
#endif /* CHIP_DEVICE_CONFIG_ENABLE_WPA */

#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
#include "edgefast_bluetooth_config.h"
#endif /* CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE */
