/*
 *
 *    Copyright (c) 2021-2022 Project CHIP Authors
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
 *
 *    Copyright 2022 NXP
 */
#include "DeviceAttestationCreds.h"

#include <crypto/CHIPCryptoPAL.h>
#include <lib/core/CHIPError.h>
#include <lib/support/Span.h>
#include <trusty_matter.h>

//TODO
  #include "credentials/examples/DeviceAttestationCredsExample.h"
  #include <credentials/examples/ExampleDACs.h>
  #include <credentials/examples/ExamplePAI.h>

using namespace matter;

namespace chip {
namespace Credentials {
namespace Trusty {

namespace {

class TrustyDACProvider : public DeviceAttestationCredentialsProvider
{
public:
    CHIP_ERROR GetCertificationDeclaration(MutableByteSpan & out_cd_buffer) override;
    CHIP_ERROR GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer) override;
    CHIP_ERROR GetDeviceAttestationCert(MutableByteSpan & out_dac_buffer) override;
    CHIP_ERROR GetProductAttestationIntermediateCert(MutableByteSpan & out_pai_buffer) override;
    CHIP_ERROR SignWithDeviceAttestationKey(const ByteSpan & message_to_sign, MutableByteSpan & out_signature_buffer) override;
    int Connect();

private:
    TrustyMatter trusty_matter;
};

CHIP_ERROR TrustyDACProvider::GetDeviceAttestationCert(MutableByteSpan & out_dac_buffer)
{
    size_t out_size = 0;
    int rc;

    rc = trusty_matter.ExportDACCert(out_dac_buffer.data(), out_dac_buffer.size(), out_size);
    if (rc == 0) {
        out_dac_buffer.reduce_size(out_size);
        return CHIP_NO_ERROR;
    } else
        return CHIP_ERROR_CERT_LOAD_FAILED;
}

CHIP_ERROR TrustyDACProvider::GetProductAttestationIntermediateCert(MutableByteSpan & out_pai_buffer)
{
    size_t out_size = 0;
    int rc;

    rc = trusty_matter.ExportPAICert(out_pai_buffer.data(), out_pai_buffer.size(), out_size);
    if (rc == 0) {
        out_pai_buffer.reduce_size(out_size);
        return CHIP_NO_ERROR;
    } else
        return CHIP_ERROR_CERT_LOAD_FAILED;
}

CHIP_ERROR TrustyDACProvider::GetCertificationDeclaration(MutableByteSpan & out_cd_buffer)
{
    size_t out_size = 0;
    int rc;

    rc = trusty_matter.ExportCDCert(out_cd_buffer.data(), out_cd_buffer.size(), out_size);
    if (rc == 0) {
        out_cd_buffer.reduce_size(out_size);
        return CHIP_NO_ERROR;
    } else
        return CHIP_ERROR_CERT_LOAD_FAILED;
}

CHIP_ERROR TrustyDACProvider::GetFirmwareInformation(MutableByteSpan & out_firmware_info_buffer)
{
    // TODO: We need a real example FirmwareInformation to be populated.
    out_firmware_info_buffer.reduce_size(0);

    return CHIP_NO_ERROR;
}

CHIP_ERROR LoadKeypairFromRaw(ByteSpan private_key, ByteSpan public_key, Crypto::P256Keypair & keypair)
{
    Crypto::P256SerializedKeypair serialized_keypair;
    ReturnErrorOnFailure(serialized_keypair.SetLength(private_key.size() + public_key.size()));
    memcpy(serialized_keypair.Bytes(), public_key.data(), public_key.size());
    memcpy(serialized_keypair.Bytes() + public_key.size(), private_key.data(), private_key.size());
    return keypair.Deserialize(serialized_keypair);
}

CHIP_ERROR TrustyDACProvider::SignWithDeviceAttestationKey(const ByteSpan & message_to_sign,
                                                            MutableByteSpan & out_signature_buffer)
{
    //TODO the sign should happen in TA
    //return CHIP_NO_ERROR;
    Crypto::P256ECDSASignature signature;
    Crypto::P256Keypair keypair;

    VerifyOrReturnError(IsSpanUsable(out_signature_buffer), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(IsSpanUsable(message_to_sign), CHIP_ERROR_INVALID_ARGUMENT);
    VerifyOrReturnError(out_signature_buffer.size() >= signature.Capacity(), CHIP_ERROR_BUFFER_TOO_SMALL);

    // In a non-exemplary implementation, the public key is not needed here. It is used here merely because
    // Crypto::P256Keypair is only (currently) constructable from raw keys if both private/public keys are present.
    ReturnErrorOnFailure(LoadKeypairFromRaw(DevelopmentCerts::kDacPrivateKey, DevelopmentCerts::kDacPublicKey, keypair));
    ReturnErrorOnFailure(keypair.ECDSA_sign_msg(message_to_sign.data(), message_to_sign.size(), signature));

    return CopySpanToMutableSpan(ByteSpan{ signature.ConstBytes(), signature.Length() }, out_signature_buffer);
}

int TrustyDACProvider::Connect()
{
    return trusty_matter.ConnectTrusty();
}

} // namespace

DeviceAttestationCredentialsProvider * GetTrustyDACProvider()
{
    static TrustyDACProvider trusty_dac_provider;
    int rc;

    //TODO connect TIPC
    rc = trusty_dac_provider.Connect();
    if (rc != 0) {
        return nullptr;
    }

    return &trusty_dac_provider;
}

} // namespace Trusty
} // namespace Credentials
} // namespace chip
