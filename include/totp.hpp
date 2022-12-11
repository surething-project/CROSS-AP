#ifndef _TOTP_H
#define _TOTP_H

#include <Arduino.h>
#include <Crypto.h>
#include <SHA512.h>
extern "C" {
    #include "base32.h"
}

class TOTP
{
  public:
    TOTP(uint8_t *hmacKey, int keyLength, int timeStep, int digits);
    TOTP(const char* secret, int timeStep, int digits);
    void getCode(long timeStamp, char* out);
    void getCodeFromSteps(long steps, char* out);
    ~TOTP();

  private:
    uint8_t* _hmacKey;
    int _keyLength;
    int _timeStep;
    int _digits;
    SHA512 sha512;
    bool _hmacKeyIsOurs;

    void putBigEndianUint64(uint8_t* eightBytes, uint64_t value);
};

#endif