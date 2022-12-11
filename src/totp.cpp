#include "totp.hpp"

TOTP::TOTP(uint8_t *hmacKey, int keyLength, int timeStep, int digits) : _hmacKey(hmacKey), _keyLength(keyLength), _timeStep(timeStep), _digits(digits), _hmacKeyIsOurs(false) {};

TOTP::TOTP(const char* secret, int timeStep, int digits) : _timeStep(timeStep), _digits(digits) {
    size_t secretLen = strlen(secret);
    _keyLength = UNBASE32_LEN(secretLen);
    _hmacKey = (uint8_t*)malloc(_keyLength);
    _hmacKeyIsOurs = true;
    _keyLength = base32_decode((const unsigned char*)secret, _hmacKey);
}

TOTP::~TOTP() {
    if(_hmacKeyIsOurs) {
        free(_hmacKey);
    }
}

void TOTP::putBigEndianUint64(uint8_t *eightBytes, uint64_t value)
{
    eightBytes[0] = (uint8_t)((value >> 56) & 0xFF);
    eightBytes[1] = (uint8_t)((value >> 48) & 0xFF);
    eightBytes[2] = (uint8_t)((value >> 40) & 0xFF);
    eightBytes[3] = (uint8_t)((value >> 32) & 0xFF);
    eightBytes[4] = (uint8_t)((value >> 24) & 0xFF);
    eightBytes[5] = (uint8_t)((value >> 16) & 0xFF);
    eightBytes[6] = (uint8_t)((value >> 8) & 0xFF);
    eightBytes[7] = (uint8_t)(value & 0xFF);
}

void TOTP::getCode(long timeStamp, char *out)
{
    long steps = timeStamp / _timeStep;
    getCodeFromSteps(steps, out);
}

void TOTP::getCodeFromSteps(long steps, char *out)
{
    // STEP 0, map the number of steps in a 8-bytes array (counter value)
    uint8_t buf[8];
    putBigEndianUint64(buf, steps);

    // STEP 1, get the HMAC-SHA512 hash from counter and key
    sha512.resetHMAC(_hmacKey, _keyLength);
    sha512.update(buf, 8);
    uint8_t sum[sha512.hashSize()];
    sha512.finalizeHMAC(_hmacKey, _keyLength, sum, sha512.hashSize());

    // STEP 2, "Dynamic truncation" in RFC 4226
    uint8_t offset = sum[sha512.hashSize() - 1] & 0xF;
    int64_t value =
        ((sum[offset] & 0x7f) << 24) |
        ((sum[offset + 1] & 0xff) << 16) |
        ((sum[offset + 2] & 0xff) << 8) |
        (sum[offset + 3] & 0xff);

    // STEP 3, compute the OTP value
    int32_t mod = value % (int32_t)pow10(_digits);

    // convert the value in string, with heading zeroes
    char format[15];
    sprintf(format, "%%0%dld", _digits);
    sprintf(out, format, mod);
}