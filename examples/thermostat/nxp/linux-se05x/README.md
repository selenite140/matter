## SE05x Feature header file

Based on the varient of the secure element connected, enable/disable the options in SE05x feature header file - `connectedhomeip/third_party/simw-top-mini/repo/fsl_sss_ftr.h`.
For details about the features supported by different secure element variants, please refer to the Product Documentation.

```c
/** SE050 Type A (ECC) */
#define SSS_HAVE_APPLET_SE05x_A 0

/** SE050 Type B (RSA) */
#define SSS_HAVE_APPLET_SE05x_B 0

/** SE050 (Super set of A + B) */
#define SSS_HAVE_APPLET_SE05x_C 0

/** SE051 with SPAKE Support */
#define SSS_HAVE_APPLET_SE051_H 0

/** SE050E */
#define SSS_HAVE_APPLET_SE050_E 1
```

```c
/** SE050 */
#define SSS_HAVE_SE05x_VER_03_XX 0

/** SE051 */
#define SSS_HAVE_SE05x_VER_06_00 0

/** SE051 */
#define SSS_HAVE_SE05x_VER_07_02 1
```

## HSM Config File

HSM config file can be used to enable/disable the required crypto oprtations to be offloaded to HSM.
File - [CHIPCryptoPALHsm_config.h](../../../../src/crypto/hsm/CHIPCryptoPALHsm_config.h)

```c
/*
 * Enable HSM for SPAKE VERIFIER
 */
#define ENABLE_HSM_SPAKE_VERIFIER 1

/*
 * Enable HSM for SPAKE PROVER
 */
#define ENABLE_HSM_SPAKE_PROVER 0

/*
 * Enable HSM for Generate EC Key
 */
#define ENABLE_HSM_GENERATE_EC_KEY 1

/*
 * Enable HSM for PBKDF SHA256
 */
#define ENABLE_HSM_PBKDF2_SHA256 0

/*
 * Enable HSM for HKDF SHA256
 */
#define ENABLE_HSM_HKDF_SHA256 0

/*
 * Enable HSM for HMAC SHA256
 */
#define ENABLE_HSM_HMAC_SHA256 1

#if ((CHIP_CRYPTO_HSM) && ((ENABLE_HSM_SPAKE_VERIFIER) || (ENABLE_HSM_SPAKE_PROVER)))
#define ENABLE_HSM_SPAKE
#endif

#if ((CHIP_CRYPTO_HSM) && (ENABLE_HSM_GENERATE_EC_KEY))
#define ENABLE_HSM_EC_KEY
//#define ENABLE_HSM_ECDSA_VERIFY
//#define ENABLE_HSM_DEVICE_ATTESTATION
#endif
```

```
1) Enable spake verifier / prover only when SE051_CHIP is selected.
2) Offloading ecdsa verify to HSM is disabled by default. To enable, Uncomment the line - `//#define ENABLE_HSM_ECDSA_VERIFY`
```

## Using SE05x for Device Attestation

To use SE05x for device attestation, provision the attestation key at location - '0x7D300000' and the device attestation certificate at location '0x7D300001'.
simw-top repo contains a example to provision the attestation key and certificate. Build / execute the example  to provision the attestation key and certificate.

```
cd connectedhomeip/third_party/simw-top-mini/repo/demos/SE05x_dev_attest_key_prov/linux/
gn gen out/debug
ninja -C out/debug
./out/debug/se05x_dev_attest_key_prov
```

Change the attestation key and certificate if required at location - `connectedhomeip/third_party/simw-top-mini/repo/demos/SE05x_dev_attest_key_prov/common/SE05x_dev_attest_key_prov.cpp`

Ensure that `ENABLE_HSM_GENERATE_EC_KEY` is enabled and `ENABLE_HSM_DEVICE_ATTESTATION` is uncommented in HSM config file.


## Build

To cross-compile this example on x64 host and run on **NXP i.MX 8M Mini**
**EVK**, see the associated
[README document](../../../../docs/guides/nxp_imx8m_linux_examples.md) for
details.
