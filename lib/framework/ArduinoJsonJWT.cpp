#include "ArduinoJsonJWT.h"

ArduinoJsonJWT::ArduinoJsonJWT(const char* secret) 
{
  strncpy(_secret, secret, JWT_MAX_SIZE);
}

void ArduinoJsonJWT::setSecret(const char* secret) 
{
  strncpy(_secret, secret, JWT_MAX_SIZE);
}

size_t ArduinoJsonJWT::getSecret(char* secret) 
{
  if(secret)
  {
    strncpy(secret, _secret, JWT_MAX_SIZE);
    return strlen(_secret);
  }

  return 0;
}

/*
 * ESP32 uses mbedtls, ESP2866 uses bearssl.
 *
 * Both come with decent HMAC implmentations supporting sha256, as well as others.
 *
 * No need to pull in additional crypto libraries - lets use what we already have.
 */
size_t ArduinoJsonJWT::sign(const char* value, size_t len, char* outstring) 
{
  unsigned char hmacResult[32];

  {
#ifdef ESP32
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
    mbedtls_md_hmac_starts(&ctx, (unsigned char*)_secret.c_str(), _secret.length());
    mbedtls_md_hmac_update(&ctx, (unsigned char*)payload.c_str(), payload.length());
    mbedtls_md_hmac_finish(&ctx, hmacResult);
    mbedtls_md_free(&ctx);
#elif defined(ESP8266)
    br_hmac_key_context keyCtx;
    br_hmac_key_init(&keyCtx, &br_sha256_vtable, _secret, strlen(_secret));
    br_hmac_context hmacCtx;
    br_hmac_init(&hmacCtx, &keyCtx, 0);
    br_hmac_update(&hmacCtx, value, len);
    br_hmac_out(&hmacCtx, hmacResult);
#endif
  }

  //TODO
  //return encode((char*)hmacResult, 32);
  return encode(reinterpret_cast<const char*>(hmacResult), 32, outstring);
}

size_t ArduinoJsonJWT::buildJWT(JsonObject& payload, char* out, size_t len) {
  // Allocate memory for the JWT
  char jwt[256]; // Adjust size as needed
  
  // Serialize payload to JWT buffer
  size_t payloadLength = serializeJson(payload, jwt, sizeof(jwt));
  
  // Encode payload
  char encodedPayload[256]; // Adjust size as needed
  encode(jwt, payloadLength, encodedPayload);
  
  // Construct the header and payload part
  char headerAndPayload[512]; // Adjust size as needed
  snprintf(headerAndPayload, sizeof(headerAndPayload), "%s.%s", JWT_HEADER, encodedPayload);
  
  // Sign the header and payload
  char signature[256]; // Adjust size as needed
  sign(headerAndPayload, strlen(headerAndPayload), signature);
  
  // Construct the full JWT
  snprintf(out, len, "%s.%s", headerAndPayload, signature);
  
  return strlen(out);
}

void ArduinoJsonJWT::parseJWT(const char* jwt, size_t len, JsonDocument& jsonDocument) {
  // clear json document before we begin, jsonDocument will be null on failure
  jsonDocument.clear();

  // must have the correct header and delimiter
  if (strncmp(jwt, JWT_HEADER, JWT_HEADER_SIZE) != 0 || jwt[JWT_HEADER_SIZE] != '.') {
    return;
  }

  // check there is a signature delimiter
  const char* signatureDelimiter = strrchr(jwt, '.');
  if (signatureDelimiter == jwt + JWT_HEADER_SIZE) {
    return;
  }

  // check the signature is valid
  size_t payloadLen = signatureDelimiter - jwt;
  char signatureBuffer[256]; // Adjust size as needed
  sign(jwt, payloadLen, signatureBuffer);

  if (strcmp(signatureBuffer, signatureDelimiter + 1) != 0) {
    return;
  }

  // decode payload
  const char* payload = jwt + JWT_HEADER_SIZE + 1;
  size_t encodedPayloadLen = payloadLen - (JWT_HEADER_SIZE + 1);
  char decodedPayload[256]; // Adjust size as needed
  size_t decodedLen = decode(payload, encodedPayloadLen, decodedPayload);

  // parse payload, clearing json document after failure
  DeserializationError error = deserializeJson(jsonDocument, decodedPayload, decodedLen);
  if (error != DeserializationError::Ok || !jsonDocument.is<JsonObject>()) {
    jsonDocument.clear();
  }
}

size_t ArduinoJsonJWT::encode(const char* cstr, size_t inputLen, char* outstring) {
  // prepare encoder
  base64_encodestate _state;
#ifdef ESP32
  base64_init_encodestate(&_state);
  size_t encodedLength = base64_encode_expected_len(inputLen) + 1;
#elif defined(ESP8266)
  base64_init_encodestate_nonewlines(&_state);
  //size_t encodedLength = base64_encode_expected_len_nonewlines(inputLen) + 1;
#endif

  // encode to outstring
  int len = base64_encode_block(cstr, inputLen, outstring, &_state);
  len += base64_encode_blockend(&outstring[len], &_state);
  //outstring[len] = '\0';

  // remove padding and convert to URL safe form
  size_t outLen = len;
  while (outLen > 0 && outstring[outLen - 1] == '=') {
    //outstring[--outLen] = '\0';
    outLen--;
  }
  outstring[outLen] = '\0';

  for (size_t i = 0; i < outLen; i++) {
    if (outstring[i] == '+') outstring[i] = '-';
    else if (outstring[i] == '/') outstring[i] = '_';
  }

  // return length of encoded string
  return outLen;
}

size_t ArduinoJsonJWT::decode(const char* cstr, size_t len, char* outstring) {
  // Convert to standard base64 in-place
  char* tempBuffer = (char*)malloc(len + 1);
  if (tempBuffer == nullptr) {
    return 0; // Memory allocation failed
  }
  
  memcpy(tempBuffer, cstr, len);
  tempBuffer[len] = '\0';
  
  for (size_t i = 0; i < len; i++) {
    if (tempBuffer[i] == '-') tempBuffer[i] = '+';
    else if (tempBuffer[i] == '_') tempBuffer[i] = '/';
  }

  // Decode
  size_t decodedLen = base64_decode_chars(tempBuffer, len, outstring);
  outstring[decodedLen] = '\0';

  free(tempBuffer);

  // Return length of decoded string
  return decodedLen;
}