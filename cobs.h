#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Worst-case encoded length for a payload of N bytes.
 * Use this to size the encode output buffer.
 */
#define COBS_ENCODED_MAX_SIZE(n) ((n) + ((n) / 254u) + 1u)

/** Maximum raw (pre-encode) payload the streaming parser will buffer. */
#define COBS_PARSER_BUF_SIZE 256u

/**
 * Encode a buffer with COBS.  The result does not include the 0x00 frame
 * delimiters — the caller wraps the output in [0x00 ... 0x00] when transmitting.
 *
 * @param data         Payload to encode.
 * @param data_len     Length of the payload.
 * @param encoded      Output buffer (must hold at least COBS_ENCODED_MAX_SIZE(data_len) bytes).
 * @param encoded_max  Capacity of the output buffer.
 * @return             Number of encoded bytes written, or 0 on any error.
 */
size_t cobs_encode(const uint8_t *data, size_t data_len,
                   uint8_t *encoded, size_t encoded_max);

/**
 * Decode a COBS-encoded buffer (without the surrounding 0x00 frame delimiters).
 *
 * @param encoded      Input buffer (raw bytes between two 0x00 delimiters).
 * @param encoded_len  Number of bytes in the input buffer.
 * @param decoded      Output buffer for the recovered payload.
 * @param decoded_max  Capacity of the output buffer.
 * @return             Number of decoded bytes written, or 0 on any error.
 */
size_t cobs_decode(const uint8_t *encoded, size_t encoded_len,
                   uint8_t *decoded, size_t decoded_max);

/**
 * Callback invoked by cobs_parser_feed() when a complete frame is decoded.
 *
 * @param decoded    Decoded payload bytes.
 * @param len        Length of the decoded payload.
 * @param user_data  Opaque pointer passed through from cobs_parser_feed().
 */
typedef void (*cobs_frame_cb_t)(const uint8_t *decoded, size_t len, void *user_data);

/**
 * Streaming COBS parser state.  Initialise with cobs_parser_init() before use.
 */
typedef struct {
    uint8_t buf[COBS_PARSER_BUF_SIZE];
    size_t  len;
} cobs_parser_t;

/**
 * Reset parser state.  Call once before the first cobs_parser_feed().
 */
void cobs_parser_init(cobs_parser_t *parser);

/**
 * Feed one byte into the parser.
 *
 * A 0x00 byte marks a frame boundary.  When a non-empty frame is completed,
 * the accumulated bytes are COBS-decoded and @p cb is invoked with the result.
 *
 * @param parser     Parser state.
 * @param byte       Next byte from the stream.
 * @param cb         Called with the decoded frame payload (may be NULL).
 * @param user_data  Passed through to @p cb unchanged.
 */
void cobs_parser_feed(cobs_parser_t *parser, uint8_t byte,
                      cobs_frame_cb_t cb, void *user_data);

#ifdef __cplusplus
}
#endif
