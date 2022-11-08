/*
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

#include "PersistentStorageOperationalKeystoreHSM.h"
#include "CHIPCryptoPALHsm_SE05X_utils.h"
#include <crypto/hsm/CHIPCryptoPALHsm.h>

#if ENABLE_HSM_GENERATE_EC_KEY

namespace chip {

using namespace chip::Crypto;

#define MAX_KEYID_SLOTS_FOR_FABRICS 32
#define FABRIC_SE05X_KEYID_START 0x7D100000

/**
 * Known issues:
 * 1. Logic to read the HSM and create the fabricTable from the persistent keys after reboot is missing .
 */

struct keyidFabIdMapping_t
{
    uint32_t keyId;
    FabricIndex fabricIndex;
    bool isPending;
    Crypto::P256KeypairHSM * pkeyPair;
} keyidFabIdMapping[MAX_KEYID_SLOTS_FOR_FABRICS] = {
    0,
};


#define ENABLE_PERSISTENT_FABRIC_KEY_TABLE 0

#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE

#define SE05x_KEY_ID_LEN 4
#define FABRIC_ID_LEN sizeof(FabricIndex)
#define SE05X_FABRIC_KEYID_MAP_BINFILE_SIZE ((SE05x_KEY_ID_LEN + FABRIC_ID_LEN) * MAX_KEYID_SLOTS_FOR_FABRICS)

/* Read data from 'kKeyId_fabricKey_table_keyid' binary file and populate keyidFabIdMapping */
CHIP_ERROR readPersistentFabricKeyTable()
{
    sss_object_t PersistFabricKeyTableKeyObj          = { 0 };
    sss_status_t status                               = kStatus_SSS_Fail;
    uint8_t cert[SE05X_FABRIC_KEYID_MAP_BINFILE_SIZE] = {
        0,
    };
    size_t certlen    = sizeof(cert);
    size_t certBitLen = sizeof(cert) * 8;
    size_t i          = 0;

    se05x_sessionOpen();

    status = sss_key_object_init(&PersistFabricKeyTableKeyObj, &gex_sss_chip_ctx.ks);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    status = sss_key_object_get_handle(&PersistFabricKeyTableKeyObj, kKeyId_fabric_key_table_keyid);
    if (status != kStatus_SSS_Success)
    {
        ChipLogProgress(Crypto, "SE05x: PersistentFabricKeyTable binary file not found. !!!");
        return CHIP_ERROR_INTERNAL;
    }

    status = sss_key_store_get_key(&gex_sss_chip_ctx.ks, &PersistFabricKeyTableKeyObj, cert, &certlen, &certBitLen);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    if (certlen != 0 && certlen % (SE05x_KEY_ID_LEN + FABRIC_ID_LEN) == 0)
    {
        while ((i + (SE05x_KEY_ID_LEN + FABRIC_ID_LEN)) <= certlen)
        {
            uint32_t keyId          = ((cert[i] << 24) + (cert[i + 1] << 16) + (cert[i + 2] << 8) + (cert[i + 3] << 0));
            FabricIndex fabricIndex = cert[i + 4];
            i                       = i + (SE05x_KEY_ID_LEN + FABRIC_ID_LEN);

            if(!IsValidFabricIndex(fabricIndex) || keyId == kKeyId_NotInitialized){
                continue;
            }

            ChipLogProgress(Crypto, "SE05x: Found key-id %02x for fabric %02x in PersistentFabricKeyTable !!!", keyId, fabricIndex);

            {
                SE05x_Result_t exists = kSE05x_Result_NA;

                VerifyOrReturnError(
                    SM_OK ==
                        Se05x_API_CheckObjectExists(&((sss_se05x_session_t *) &gex_sss_chip_ctx.session)->s_ctx, keyId, &exists),
                    CHIP_ERROR_INTERNAL);
                if (exists != kSE05x_Result_SUCCESS)
                {
                    ChipLogProgress(Crypto, "SE05x: Key-id %02x not found in SE05x. Ignoring the key", keyId);
                    continue;
                }

                Crypto::P256KeypairHSM * pkeyPair = Platform::New<Crypto::P256KeypairHSM>();
                if (pkeyPair != nullptr)
                {
                    pkeyPair->SetKeyId(keyId);
                    pkeyPair->provisioned_key = true; /* Set provisioned_key = true, to make sure key is not regenerated. */
                    pkeyPair->Initialize();
                    pkeyPair->provisioned_key =
                        false; /* Set provisioned_key = false, to make sure key can be deleted when required */
                }

                uint32_t slotId = keyId - FABRIC_SE05X_KEYID_START;
                if (slotId >= MAX_KEYID_SLOTS_FOR_FABRICS)
                {
                    ChipLogProgress(Crypto, "SE05x: Invalid slot id - %02x. Ignoring the respective key-id - %02x", slotId, keyId);
                    continue;
                }

                if (keyidFabIdMapping[slotId].keyId != kKeyId_NotInitialized)
                {
                    ChipLogProgress(Crypto, "SE05x: Multiple keys found for slot id %02x. Overwriting the previous one !!!",
                                    slotId);
                }

                ChipLogProgress(Crypto, "SE05x: Storing fabricIndex=%02x and key-id=%02x keyidFabIdMapping table !!!", fabricIndex,
                                keyId);
                keyidFabIdMapping[slotId].keyId       = keyId;
                keyidFabIdMapping[slotId].fabricIndex = fabricIndex;
                keyidFabIdMapping[slotId].isPending   = false;
                keyidFabIdMapping[slotId].pkeyPair    = pkeyPair;
            }
        }
    }
    else
    {
        return CHIP_ERROR_INTERNAL;
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR writeToPersistentFabricKeyTable(uint32_t keyId, FabricIndex fabricIndex)
{
    sss_object_t PersistFabricKeyTableKeyObj          = { 0 };
    sss_status_t status                               = kStatus_SSS_Fail;
    uint8_t cert[SE05X_FABRIC_KEYID_MAP_BINFILE_SIZE] = {
        0,
    };
    size_t certlen      = sizeof(cert);
    size_t certlen_bits = sizeof(cert) * 8;
    size_t offset       = keyId - FABRIC_SE05X_KEYID_START;

    se05x_sessionOpen();

    status = sss_key_object_init(&PersistFabricKeyTableKeyObj, &gex_sss_chip_ctx.ks);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    status = sss_key_object_get_handle(&PersistFabricKeyTableKeyObj, kKeyId_fabric_key_table_keyid);
    if (status != kStatus_SSS_Success)
    {
        ChipLogProgress(Crypto, "SE05x: PersistentFabricKeyTable binary file not found. Creating one now !!!");

        status = sss_key_object_allocate_handle(
            &PersistFabricKeyTableKeyObj, kKeyId_fabric_key_table_keyid, kSSS_KeyPart_Default, kSSS_CipherType_Certificate,
            ((SE05x_KEY_ID_LEN + FABRIC_ID_LEN) * MAX_KEYID_SLOTS_FOR_FABRICS), kKeyObject_Mode_Persistent);
        VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

        // Set the key info in binary file data
        cert[offset]     = (uint8_t)(keyId >> 24) & 0xFF;
        cert[offset + 1] = (uint8_t)(keyId >> 16) & 0xFF;
        cert[offset + 2] = (uint8_t)(keyId >> 8) & 0xFF;
        cert[offset + 3] = (uint8_t)(keyId & 0xFF);
        cert[offset + 4] = (uint8_t) fabricIndex;

        status = sss_key_store_set_key(&gex_sss_chip_ctx.ks, &PersistFabricKeyTableKeyObj, cert, certlen, certlen * 8, NULL, 0);
        VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

        ChipLogProgress(Crypto, "SE05x: PersistentFabricKeyTable Binary file created ( with fabric=%02x and key-id = %02xinfo)",
                        fabricIndex, keyId);

        return CHIP_NO_ERROR;
    }

    status = sss_key_store_get_key(&gex_sss_chip_ctx.ks, &PersistFabricKeyTableKeyObj, cert, &certlen, &certlen_bits);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    // Set the key info in binary file data
    cert[offset]     = (uint8_t)(keyId >> 24) & 0xFF;
    cert[offset + 1] = (uint8_t)(keyId >> 16) & 0xFF;
    cert[offset + 2] = (uint8_t)(keyId >> 8) & 0xFF;
    cert[offset + 3] = (uint8_t)(keyId & 0xFF);
    cert[offset + 4] = (uint8_t) fabricIndex;

    status = sss_key_store_set_key(&gex_sss_chip_ctx.ks, &PersistFabricKeyTableKeyObj, cert, certlen, certlen * 8, NULL, 0);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    ChipLogProgress(Crypto, "SE05x: PersistentFabricKeyTable Binary file updated ( added fabric=%02x and key-id = %02xinfo)",
                    fabricIndex, keyId);

    return CHIP_NO_ERROR;
}

CHIP_ERROR removeFromPersistentFabricKeyTable(uint32_t keyId, FabricIndex fabricIndex)
{
    sss_object_t PersistFabricKeyTableKeyObj          = { 0 };
    sss_status_t status                               = kStatus_SSS_Fail;
    uint8_t cert[SE05X_FABRIC_KEYID_MAP_BINFILE_SIZE] = {
        0,
    };
    size_t certlen      = sizeof(cert);
    size_t certlen_bits = sizeof(cert) * 8;
    size_t offset       = keyId - FABRIC_SE05X_KEYID_START;

    se05x_sessionOpen();

    status = sss_key_object_init(&PersistFabricKeyTableKeyObj, &gex_sss_chip_ctx.ks);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    status = sss_key_object_get_handle(&PersistFabricKeyTableKeyObj, kKeyId_fabric_key_table_keyid);
    if (status != kStatus_SSS_Success)
    {
        ChipLogProgress(Crypto, "SE05x: PersistentFabricKeyTable binary file not found.!!!");
        return CHIP_ERROR_INTERNAL;
    }

    status = sss_key_store_get_key(&gex_sss_chip_ctx.ks, &PersistFabricKeyTableKeyObj, cert, &certlen, &certlen_bits);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    // Remove the key info in binary file data
    memset((cert + offset), 0, (SE05x_KEY_ID_LEN + FABRIC_ID_LEN));

    status = sss_key_store_set_key(&gex_sss_chip_ctx.ks, &PersistFabricKeyTableKeyObj, cert, certlen, certlen * 8, NULL, 0);
    VerifyOrReturnError(status == kStatus_SSS_Success, CHIP_ERROR_INTERNAL);

    ChipLogProgress(Crypto, "SE05x: PersistentFabricKeyTable Binary file updated ( removed fabric=%02x and key-id = %02xinfo)",
                    fabricIndex, keyId);

    return CHIP_NO_ERROR;
}

#endif //#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE

uint8_t getEmpytSlotId()
{
    uint8_t i = 0;
    for (auto & mapping : keyidFabIdMapping)
    {
        if (mapping.keyId == kKeyId_NotInitialized && mapping.isPending == false)
        {
            break;
        }
        i++;
    }
    return i;
}

PersistentStorageOperationalKeystoreHSM::PersistentStorageOperationalKeystoreHSM()
{
#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE
    CHIP_ERROR err = readPersistentFabricKeyTable();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogProgress(Crypto, "Error in readPersistentFabricKeyTable");
    }
#endif //#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE
}

void PersistentStorageOperationalKeystoreHSM::ResetPendingSlot()
{
    uint32_t slotId = mPendingKeypair->GetKeyId() - FABRIC_SE05X_KEYID_START;
    if (slotId < MAX_KEYID_SLOTS_FOR_FABRICS)
    {
        keyidFabIdMapping[slotId].keyId       = kKeyId_NotInitialized;
        keyidFabIdMapping[slotId].fabricIndex = kUndefinedFabricIndex;
        keyidFabIdMapping[slotId].isPending   = false;
    }
}

bool PersistentStorageOperationalKeystoreHSM::HasOpKeypairForFabric(FabricIndex fabricIndex) const
{
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), false);

    // If there was a pending keypair, then there's really a usable key
    if (mIsPendingKeypairActive && (fabricIndex == mPendingFabricIndex) && (mPendingKeypair != nullptr))
    {
        ChipLogProgress(Crypto, "SE05x: HasOpKeypairForFabric ==> mPendingKeypair found");
        return true;
    }

    for (auto & mapping : keyidFabIdMapping)
    {
        if (mapping.fabricIndex == fabricIndex)
        {
            ChipLogProgress(Crypto, "SE05x: HasOpKeypairForFabric ==> stored keyPair found");
            return true;
        }
    }

    ChipLogProgress(Crypto, "SE05x: HasOpKeypairForFabric ==> No key found");
    return false;
}

CHIP_ERROR PersistentStorageOperationalKeystoreHSM::NewOpKeypairForFabric(FabricIndex fabricIndex,
                                                                          MutableByteSpan & outCertificateSigningRequest)
{
    CHIP_ERROR err = CHIP_NO_ERROR;

    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    ChipLogProgress(Crypto, "SE05x: New Op Keypair for Fabric %02x", fabricIndex);

    // Replace previous pending keypair, if any was previously allocated
    ResetPendingKey();

    uint8_t slotId = getEmpytSlotId();
    VerifyOrReturnError(slotId < MAX_KEYID_SLOTS_FOR_FABRICS, CHIP_ERROR_NO_MEMORY);

    mPendingKeypair = Platform::New<Crypto::P256KeypairHSM>();
    VerifyOrReturnError(mPendingKeypair != nullptr, CHIP_ERROR_NO_MEMORY);

    // Key id is created as slotid + start offset of ops key id
    mPendingKeypair->SetKeyId(FABRIC_SE05X_KEYID_START + slotId);
    mPendingKeypair->is_persistent = true;

    err = mPendingKeypair->Initialize();
    VerifyOrReturnError(err == CHIP_NO_ERROR, CHIP_ERROR_NO_MEMORY);

    mPendingFabricIndex = fabricIndex;

    keyidFabIdMapping[slotId].isPending = true;

    size_t csrLength = outCertificateSigningRequest.size();
    err              = mPendingKeypair->NewCertificateSigningRequest(outCertificateSigningRequest.data(), csrLength);
    if (err != CHIP_NO_ERROR)
    {
        ResetPendingKey();
        return err;
    }
    outCertificateSigningRequest.reduce_size(csrLength);

    return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentStorageOperationalKeystoreHSM::ActivateOpKeypairForFabric(FabricIndex fabricIndex,
                                                                               const Crypto::P256PublicKey & nocPublicKey)
{
    ChipLogProgress(Crypto, "SE05x: ActivateOpKeypair for Fabric %02x", fabricIndex);

    VerifyOrReturnError(mPendingKeypair != nullptr, CHIP_ERROR_INVALID_FABRIC_INDEX);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex) && (fabricIndex == mPendingFabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    // Validate public key being activated matches last generated pending keypair
    VerifyOrReturnError(mPendingKeypair->Pubkey().Matches(nocPublicKey), CHIP_ERROR_INVALID_PUBLIC_KEY);

    mIsPendingKeypairActive = true;

    return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentStorageOperationalKeystoreHSM::CommitOpKeypairForFabric(FabricIndex fabricIndex)
{
    VerifyOrReturnError(mPendingKeypair != nullptr, CHIP_ERROR_INVALID_FABRIC_INDEX);
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex) && (fabricIndex == mPendingFabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);
    VerifyOrReturnError(mIsPendingKeypairActive == true, CHIP_ERROR_INCORRECT_STATE);

    uint32_t slotId = mPendingKeypair->GetKeyId() - FABRIC_SE05X_KEYID_START;

    VerifyOrReturnError(slotId < MAX_KEYID_SLOTS_FOR_FABRICS, CHIP_ERROR_NO_MEMORY);

    ChipLogProgress(Crypto, "SE05x: CommitOpKeypair for Fabric %02x", fabricIndex);

    for (auto & mapping : keyidFabIdMapping)
    {
        if (mapping.fabricIndex == fabricIndex)
        {
            // Delete the previous keyPair associated with the fabric
            mapping.isPending = false;
            Platform::Delete<Crypto::P256KeypairHSM>(mapping.pkeyPair);
            mapping.pkeyPair    = NULL;
            mapping.keyId       = kKeyId_NotInitialized;
            mapping.fabricIndex = kUndefinedFabricIndex;
        }
    }

    keyidFabIdMapping[slotId].pkeyPair    = mPendingKeypair;
    keyidFabIdMapping[slotId].keyId       = mPendingKeypair->GetKeyId();
    keyidFabIdMapping[slotId].fabricIndex = mPendingFabricIndex;
    keyidFabIdMapping[slotId].isPending   = false;

    // If we got here, we succeeded and can reset the pending key: next `SignWithOpKeypair` will use the stored key.
    mPendingKeypair         = nullptr;
    mIsPendingKeypairActive = false;
    mPendingFabricIndex     = kUndefinedFabricIndex;

#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE
    CHIP_ERROR err = writeToPersistentFabricKeyTable(keyidFabIdMapping[slotId].keyId, keyidFabIdMapping[slotId].fabricIndex);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogProgress(Crypto, "Error in writeToPersistentFabricKeyTable");
    }
#endif //#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE

    return CHIP_NO_ERROR;
}

CHIP_ERROR PersistentStorageOperationalKeystoreHSM::RemoveOpKeypairForFabric(FabricIndex fabricIndex)
{
    VerifyOrReturnError(IsValidFabricIndex(fabricIndex), CHIP_ERROR_INVALID_FABRIC_INDEX);

    ChipLogProgress(Crypto, "SE05x: RemoveOpKeypair for Fabric %02x", fabricIndex);

    // Remove pending state if matching
    if ((mPendingKeypair != nullptr) && (fabricIndex == mPendingFabricIndex))
    {
        RevertPendingKeypair();
    }

    for (auto & mapping : keyidFabIdMapping)
    {
        if (mapping.fabricIndex == fabricIndex)
        {
#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE
            CHIP_ERROR err = removeFromPersistentFabricKeyTable(mapping.keyId, fabricIndex);
            if (err != CHIP_NO_ERROR)
            {
                ChipLogProgress(Crypto, "Error in writeToPersistentFabricKeyTable");
            }
#endif //#if ENABLE_PERSISTENT_FABRIC_KEY_TABLE

            // Delete the keyPair associated with the fabric
            mapping.isPending = false;
            Platform::Delete<Crypto::P256KeypairHSM>(mapping.pkeyPair);
            mapping.pkeyPair    = NULL;
            mapping.keyId       = kKeyId_NotInitialized;
            mapping.fabricIndex = kUndefinedFabricIndex;
        }
    }

    return CHIP_NO_ERROR;
}

void PersistentStorageOperationalKeystoreHSM::RevertPendingKeypair()
{
    ChipLogProgress(Crypto, "SE05x: RevertPendingKeypair");
    // Just reset the pending key, we never stored anything
    ResetPendingKey();
}

CHIP_ERROR PersistentStorageOperationalKeystoreHSM::SignWithOpKeypair(FabricIndex fabricIndex, const ByteSpan & message,
                                                                      Crypto::P256ECDSASignature & outSignature) const
{
    ChipLogProgress(Crypto, "SE05x: SignWithOpKeypair");

    if (mIsPendingKeypairActive && (fabricIndex == mPendingFabricIndex))
    {
        VerifyOrReturnError(mPendingKeypair != nullptr, CHIP_ERROR_INTERNAL);
        // We have an override key: sign with it!
        ChipLogProgress(Crypto, "SE05x: SignWithOpKeypair ==> using mPendingKeypair");
        return mPendingKeypair->ECDSA_sign_msg(message.data(), message.size(), outSignature);
    }
    else
    {
        for (auto & mapping : keyidFabIdMapping)
        {
            if ((mapping.fabricIndex == fabricIndex) && (mapping.isPending == false))
            {
                ChipLogProgress(Crypto, "SE05x: SignWithOpKeypair ==> using stored keyPair");
                return mapping.pkeyPair->ECDSA_sign_msg(message.data(), message.size(), outSignature);
            }
        }
    }

    ChipLogProgress(Crypto, "SE05x: SignWithOpKeypair ==> No keyPair found");
    return CHIP_ERROR_INVALID_FABRIC_INDEX;
}

Crypto::P256Keypair * PersistentStorageOperationalKeystoreHSM::AllocateEphemeralKeypairForCASE()
{
    ChipLogProgress(Crypto, "SE05x: AllocateEphemeralKeypairForCASE using se05x");
    Crypto::P256KeypairHSM * pkeyPair = Platform::New<Crypto::P256KeypairHSM>();

    if (pkeyPair != nullptr)
    {
        pkeyPair->SetKeyId(kKeyId_case_ephemeral_keyid);
    }

    return pkeyPair;
}

void PersistentStorageOperationalKeystoreHSM::ReleaseEphemeralKeypair(Crypto::P256Keypair * keypair)
{
    ChipLogProgress(Crypto, "SE05x: ReleaseEphemeralKeypair using se05x");
    Platform::Delete(static_cast<Crypto::P256KeypairHSM *>(keypair));
}

} // namespace chip

#endif //#if ENABLE_HSM_GENERATE_EC_KEY