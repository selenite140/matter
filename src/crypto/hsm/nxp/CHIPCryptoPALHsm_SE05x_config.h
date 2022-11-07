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
