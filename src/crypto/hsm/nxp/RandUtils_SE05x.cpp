/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2013-2017 Nest Labs, Inc.
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

/**
 *    @file
 *      This file implements utility functions for deriving random integers from SE05x HSM.
 *
 *
 */

#include <crypto/RandUtils.h>

#include <stdint.h>
#include <stdlib.h>

#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/CodeUtils.h>
#include <crypto/hsm/CHIPCryptoPALHsm.h>
#include "CHIPCryptoPALHsm_SE05X_utils.h"

namespace chip {
namespace Crypto {


#ifdef ENABLE_HSM_RAND

static void se05x_generate_random(uint8_t *rndData, size_t rndDataLen)
{
    sss_status_t status;
    sss_rng_context_t rng_context = {0,};

    se05x_sessionOpen();
    VerifyOrDie(gex_sss_chip_ctx.ks.session != NULL);

    status = sss_rng_context_init(&rng_context, &gex_sss_chip_ctx.session /* Session */);
    VerifyOrDie(status == kStatus_SSS_Success);

    status = sss_rng_get_random(&rng_context, rndData, rndDataLen);
    VerifyOrDie(status == kStatus_SSS_Success);

    sss_rng_context_free(&rng_context);
}

uint64_t GetRandU64()
{
    uint64_t tmp = 0;
    se05x_generate_random(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp));
    return tmp;
}

uint32_t GetRandU32()
{
    uint32_t tmp = 0;
    se05x_generate_random(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp));
    return tmp;
}

uint16_t GetRandU16()
{
    uint16_t tmp = 0;
    se05x_generate_random(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp));
    return tmp;
}

uint8_t GetRandU8()
{
    uint8_t tmp = 0;
    se05x_generate_random(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp));
    return tmp;
}

#else

uint64_t GetRandU64()
{
    uint64_t tmp = 0;
    VerifyOrDie(CHIP_NO_ERROR == DRBG_get_bytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp)));
    return tmp;
}

uint32_t GetRandU32()
{
    uint32_t tmp = 0;
    VerifyOrDie(CHIP_NO_ERROR == DRBG_get_bytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp)));
    return tmp;
}

uint16_t GetRandU16()
{
    uint16_t tmp = 0;
    VerifyOrDie(CHIP_NO_ERROR == DRBG_get_bytes(reinterpret_cast<uint8_t *>(&tmp), sizeof(tmp)));
    return tmp;
}

uint8_t GetRandU8()
{
    uint8_t tmp = 0;
    VerifyOrDie(CHIP_NO_ERROR == DRBG_get_bytes(&tmp, sizeof(tmp)));
    return tmp;
}

#endif //#ifdef ENABLE_HSM_RAND

} // namespace Crypto
} // namespace chip
