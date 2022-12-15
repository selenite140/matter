/*
 * Copyright 2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_adapter_rng.h"
#include "ksdk_mbedtls.h"

status_t CRYPTO_InitHardware(void)
{
    hal_rng_status_t status = HAL_RngInit();
    return (status == kStatus_HAL_RngSuccess) ? 0 : kStatus_Fail;
}

int mbedtls_hardware_poll(void * data, unsigned char * output, size_t len, size_t * olen)
{
    hal_rng_status_t status;

    status = HAL_RngGetData(output, len);

    if (status == kStatus_HAL_RngSuccess)
    {
        *olen = len;
        return 0;
    }
    return kStatus_Fail;
}
