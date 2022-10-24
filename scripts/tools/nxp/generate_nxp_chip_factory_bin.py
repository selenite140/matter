#!/usr/bin/env python3
#
#    Copyright (c) 2022 Project CHIP Authors
#    All rights reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

import os
import argparse
import subprocess
import base64
import logging
import hashlib
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives.serialization import load_der_private_key

# Keys used to store KLV
KLV_KEYS = {
    "Verifier": 1,
    "Salt": 2,
    "IC": 3,
    "DacPKey": 4,
    "DacCert": 5,
    "PaiCert": 6,
    "Disc": 7
}

def type_int(n):
    return int(n, 0)

def type_base64_str(s):
    return base64.b64decode(s)

def read_der(der_file):
    with open(der_file, "rb") as file:
        return file.read()

def generate_spake2_params(args):
    cmd = [
        args.spake2p_path, "gen-verifier",
        "--iteration-count", str(args.it),
        "--salt", base64.b64encode(args.salt),
        "--pin-code", str(args.passcode),
        "--out", "-"
    ]

    out = subprocess.run(cmd, check=True, stdout=subprocess.PIPE).stdout
    out = out.decode("utf-8").splitlines()
    return dict(zip(out[0].split(','), out[1].split(',')))

def get_private_key_der(der_file, password):
    data = read_der(der_file)
    keys = load_der_private_key(data, password, backend=default_backend())
    private_key = keys.private_numbers().private_value.to_bytes(
        32, byteorder='big')
    return private_key

def generate_klv(args):
    klv = list()

    if args.spake2p_verifier is None:
        spake2p_params = generate_spake2_params(args)
        spake2p_verifier = str.encode(spake2p_params["Verifier"])
        spake2p_salt = str.encode(spake2p_params["Salt"])
        spake2p_it = int(spake2p_params["Iteration Count"]).to_bytes(4, "little")
    else:
        spake2p_verifier = base64.b64encode(args.spake2p_verifier)
        spake2p_salt = base64.b64encode(args.salt)
        spake2p_it = int(args.it).to_bytes(4, "little")

    dac_private_key = get_private_key_der(args.dac_key, args.dac_key_password)
    dac_cert = read_der(args.dac_cert)
    pai_cert = read_der(args.pai_cert)
    discriminator = int(args.discriminator).to_bytes(4, "little")

    klv.append((KLV_KEYS["Verifier"], len(spake2p_verifier), spake2p_verifier))
    klv.append((KLV_KEYS["Salt"], len(spake2p_salt), spake2p_salt))
    klv.append((KLV_KEYS["IC"], 4, spake2p_it))
    klv.append((KLV_KEYS["DacPKey"], len(dac_private_key), dac_private_key))
    klv.append((KLV_KEYS["DacCert"], len(dac_cert), dac_cert))
    klv.append((KLV_KEYS["PaiCert"], len(pai_cert), pai_cert))
    klv.append((KLV_KEYS["Disc"], 4, discriminator))

    print("Verifier length: ", len(spake2p_verifier))
    print("SALT length: ", len(spake2p_salt))
    print("DAC Private Key length: ", len(dac_private_key))
    print("DAC Certificate length: ", len(dac_cert))
    print("PAI Certificate length: ", len(pai_cert))

    return klv

def klv_to_bin(klv, out):
    with open(out, "wb") as file:
        for entry in klv:
            file.write(entry[0].to_bytes(1, "little"))
            file.write(entry[1].to_bytes(2, "little"))
            file.write(entry[2])
        size = file.seek(0, os.SEEK_END)
        print("Size of generated binary is:", size, "bytes")

    out_hash = hashlib.sha256(fullContent).hexdigest()
    print("SHA256 of generated binary:", out_hash)

def main():
    parser = argparse.ArgumentParser(description="NXP Factory Data Generator")
    optional = parser
    required = parser.add_argument_group("required arguments")

    required.add_argument("-i", "--it", required=True, type=type_int,
                        help="[int | hex] Spake2 Iteration Counter")
    required.add_argument("-s", "--salt", required=True, type=type_base64_str,
                        help="[base64 str] Spake2 Salt")
    required.add_argument("-p", "--passcode", required=True, type=type_int,
                        help="[int | hex] PASE session passcode")
    required.add_argument("-d", "--discriminator", required=True, type=type_int,
                        help="[int | hex] BLE Pairing discriminator")
    required.add_argument("--dac_cert", required=True, type=str,
                        help="[path] Path to DAC certificate in DER format")
    required.add_argument("--dac_key", required=True, type=str,
                        help="[path] Path to DAC key in DER format")
    required.add_argument("--pai_cert", required=True, type=str,
                        help="[path] Path to PAI certificate in DER format")
    required.add_argument("--spake2p_path", required=True, type=str,
                        help="[path] Path to spake2p tool")
    required.add_argument("--out", required=True, type=str,
                        help="[path] Path to output binary")

    optional.add_argument("--dac_key_password", type=str,
                        help="[path] Password to decode DAC Key if available")
    optional.add_argument("--spake2p_verifier", type=type_base64_str,
                        help="[base64 str] Already generated spake2p verifier")
    optional.add_argument("--aes128_key",
                        help="[hex] AES 128 bits key used to encrypt the whole dataset")

    args = parser.parse_args()

    if args.dac_key_password is None:
        logging.warning(
            "DAC Key password not provided. It means DAC Key is not protected."
        )

    klv = generate_klv(args)
    klv_to_bin(klv, args.out, args.aes128_key)

if __name__ == "__main__":
    main()
