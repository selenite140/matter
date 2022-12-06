/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
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
 *      Header that exposes the options to enable HSM(SE05x) for required crypto operations.
 */

#pragma once

/*
 * Use target secure object for spake2p session keys
 * Note: Make sure to enable 'ENABLE_HSM_HKDF_SHA256' in src/crypto/hsm/CHIPCryptoPALHsm_config.h file
 */
#define SE05X_SECURE_OBJECT_FOR_SPAKE2P_SESSION_KEYS 1


/**** Use the below options in case TP keys are to be used ****/

/*
 * Use TP keys for spake2p
 */
#define USE_SE05X_TRUST_PROV_KEYS_FOR_SPAKE2P_VERIFIER 0

/*
 * Verifier set to be used. Possible values == > 1,2,3
 */
#define SE05X_VERIFIER_SET_NUM 1

/*
 * Itteration count == > Possible values == > 1000, 5000, 10000, 50000, 100000
 */
#define SE05X_SPAKE2P_ITTERATION_COUNT 1000

/*
 * Use TP keys for device attestation
 */
#define USE_SE05X_TRUST_PROV_KEYS_FOR_DEV_ATTEST 1


#if ( (CHIP_CRYPTO_HSM) && (USE_SE05X_TRUST_PROV_KEYS_FOR_SPAKE2P_VERIFIER) && (ENABLE_HSM_SPAKE_VERIFIER) )
#define USE_SE05X_TP_KEYS_FOR_SPAKE2P_VERIFIER
#endif