/*
 *  Copyright (c) 2016, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <openthread/config.h>

#include "common/debug.hpp"
#include "common/message.hpp"
#include "crypto/hmac_sha256.hpp"
#include "crypto/sha256.hpp"

#include "test_platform.h"
#include "test_util.h"

namespace ot {

void TestSha256(void)
{
    const char kData1[] = "abc";

    const otCryptoSha256Hash kHash1 = {{
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
    }};

    const char kData2[] = "";

    const otCryptoSha256Hash kHash2 = {{
        0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
        0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55,
    }};

    const char kData3[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

    const otCryptoSha256Hash kHash3 = {{
        0x24, 0x8d, 0x6a, 0x61, 0xd2, 0x06, 0x38, 0xb8, 0xe5, 0xc0, 0x26, 0x93, 0x0c, 0x3e, 0x60, 0x39,
        0xa3, 0x3c, 0xe4, 0x59, 0x64, 0xff, 0x21, 0x67, 0xf6, 0xec, 0xed, 0xd4, 0x19, 0xdb, 0x06, 0xc1,
    }};

    const char kData4[] = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmno"
                          "pqrsmnopqrstnopqrstu";

    const otCryptoSha256Hash kHash4 = {{
        0xcf, 0x5b, 0x16, 0xa7, 0x78, 0xaf, 0x83, 0x80, 0x03, 0x6c, 0xe5, 0x9e, 0x7b, 0x04, 0x92, 0x37,
        0x0b, 0x24, 0x9b, 0x11, 0xe8, 0xf0, 0x7a, 0x51, 0xaf, 0xac, 0x45, 0x03, 0x7a, 0xfe, 0xe9, 0xd1,
    }};

    struct TestCase
    {
        const char *       mData; // (null-terminated string).
        otCryptoSha256Hash mHash;
    };

    static const TestCase kTestCases[] = {
        {kData1, kHash1},
        {kData2, kHash2},
        {kData3, kHash3},
        {kData4, kHash4},
    };

    printf("TestSha256\n");

    Instance *   instance = testInitInstance();
    MessagePool *messagePool;
    Message *    message;
    uint16_t     offsets[OT_ARRAY_LENGTH(kTestCases)];
    uint8_t      index;

    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();
    VerifyOrQuit((message = messagePool->New(Message::kTypeIp6, 0)) != nullptr, "Message::New failed");

    for (const TestCase &testCase : kTestCases)
    {
        Crypto::Sha256       sha256;
        Crypto::Sha256::Hash hash;

        sha256.Start();
        sha256.Update(testCase.mData, static_cast<uint16_t>(strlen(testCase.mData)));
        sha256.Finish(hash);

        VerifyOrQuit(hash == static_cast<const Crypto::HmacSha256::Hash &>(testCase.mHash), "HMAC-SHA-256 failed");
    }

    // Append all test case `mData` in the message.

    index = 0;

    for (const TestCase &testCase : kTestCases)
    {
        SuccessOrQuit(message->Append("Hello"), "Message::Append() failed");
        offsets[index++] = message->GetLength();
        SuccessOrQuit(message->AppendBytes(testCase.mData, static_cast<uint16_t>(strlen(testCase.mData))),
                      "Message::AppendBytes() failed");
        SuccessOrQuit(message->Append("There!"), "Message::Append() failed");
    }

    index = 0;

    for (const TestCase &testCase : kTestCases)
    {
        Crypto::Sha256       sha256;
        Crypto::Sha256::Hash hash;

        sha256.Start();
        sha256.Update(*message, offsets[index++], static_cast<uint16_t>(strlen(testCase.mData)));
        sha256.Finish(hash);

        VerifyOrQuit(hash == static_cast<const Crypto::HmacSha256::Hash &>(testCase.mHash), "HMAC-SHA-256 failed");
    }

    testFreeInstance(instance);
}

void TestHmacSha256(void)
{
    struct TestCase
    {
        const void *       mKey;
        uint16_t           mKeyLength;
        const void *       mData;
        uint16_t           mDataLength;
        otCryptoSha256Hash mHash;
    };

    // Test-cases from RFC 4231.

    const uint8_t kKey1[] = {
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
        0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    };

    const char kData1[] = "Hi There";

    const otCryptoSha256Hash kHash1 = {{
        0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf, 0xce, 0xaf, 0x0b, 0xf1, 0x2b,
        0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c, 0x2e, 0x32, 0xcf, 0xf7,
    }};

    const char kKey2[]  = "Jefe";
    const char kData2[] = "what do ya want for nothing?";

    const otCryptoSha256Hash kHash2 = {{
        0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24, 0x26, 0x08, 0x95, 0x75, 0xc7,
        0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec, 0x38, 0x43,
    }};

    const uint8_t kKey3[] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
                             0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};

    const uint8_t kData3[] = {
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
        0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    };

    const otCryptoSha256Hash kHash3 = {{
        0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46, 0x85, 0x4d, 0xb8, 0xeb, 0xd0, 0x91, 0x81, 0xa7,
        0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8, 0xc1, 0x22, 0xd9, 0x63, 0x55, 0x14, 0xce, 0xd5, 0x65, 0xfe,
    }};

    const uint8_t kKey4[] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
        0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    };

    const uint8_t kData4[] = {
        0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
        0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
        0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
    };

    const otCryptoSha256Hash kHash4 = {{
        0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e, 0xa4, 0xcc, 0x81, 0x98, 0x99, 0xf2, 0x08, 0x3a,
        0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78, 0xf8, 0x07, 0x7a, 0x2e, 0x3f, 0xf4, 0x67, 0x29, 0x66, 0x5b,
    }};

    const uint8_t kKey5[] = {
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
        0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    };

    const char kData5[] = "This is a test using a larger than block-size key and a larger than block-size data. The "
                          "key needs to be hashed before being used by the HMAC algorithm.";

    const otCryptoSha256Hash kHash5 = {{
        0x9b, 0x09, 0xff, 0xa7, 0x1b, 0x94, 0x2f, 0xcb, 0x27, 0x63, 0x5f, 0xbc, 0xd5, 0xb0, 0xe9, 0x44,
        0xbf, 0xdc, 0x63, 0x64, 0x4f, 0x07, 0x13, 0x93, 0x8a, 0x7f, 0x51, 0x53, 0x5c, 0x3a, 0x35, 0xe2,
    }};

    static const TestCase kTestCases[] = {
        {kKey1, sizeof(kKey1), kData1, sizeof(kData1) - 1, kHash1},
        {kKey2, sizeof(kKey2) - 1, kData2, sizeof(kData2) - 1, kHash2},
        {kKey3, sizeof(kKey3), kData3, sizeof(kData3), kHash3},
        {kKey4, sizeof(kKey4), kData4, sizeof(kData4), kHash4},
        {kKey5, sizeof(kKey5), kData5, sizeof(kData5) - 1, kHash5},
    };

    Instance *   instance = testInitInstance();
    MessagePool *messagePool;
    Message *    message;
    uint16_t     offsets[OT_ARRAY_LENGTH(kTestCases)];
    uint8_t      index;

    printf("TestHmacSha256\n");

    VerifyOrQuit(instance != nullptr, "Null OpenThread instance");

    messagePool = &instance->Get<MessagePool>();
    VerifyOrQuit((message = messagePool->New(Message::kTypeIp6, 0)) != nullptr, "Message::New failed");

    for (const TestCase &testCase : kTestCases)
    {
        Crypto::HmacSha256       hmac;
        Crypto::HmacSha256::Hash hash;

        hmac.Start(reinterpret_cast<const uint8_t *>(testCase.mKey), testCase.mKeyLength);
        hmac.Update(testCase.mData, testCase.mDataLength);
        hmac.Finish(hash);

        VerifyOrQuit(hash == static_cast<const Crypto::HmacSha256::Hash &>(testCase.mHash), "HMAC-SHA-256 failed");
    }

    // Append all test case `mData` in the message.

    index = 0;

    for (const TestCase &testCase : kTestCases)
    {
        SuccessOrQuit(message->Append("Hello"), "Message::Append() failed");
        offsets[index++] = message->GetLength();
        SuccessOrQuit(message->AppendBytes(testCase.mData, testCase.mDataLength), "Message::AppendBytes() failed");
        SuccessOrQuit(message->Append("There"), "Message::Append() failed");
    }

    index = 0;

    for (const TestCase &testCase : kTestCases)
    {
        Crypto::HmacSha256       hmac;
        Crypto::HmacSha256::Hash hash;

        hmac.Start(reinterpret_cast<const uint8_t *>(testCase.mKey), testCase.mKeyLength);
        hmac.Update(*message, offsets[index++], testCase.mDataLength);
        hmac.Finish(hash);

        VerifyOrQuit(hash == static_cast<const Crypto::HmacSha256::Hash &>(testCase.mHash), "HMAC-SHA-256 failed");
    }

    message->Free();

    testFreeInstance(instance);
}

} // namespace ot

int main(void)
{
    ot::TestSha256();
    ot::TestHmacSha256();
    printf("All tests passed\n");
    return 0;
}
