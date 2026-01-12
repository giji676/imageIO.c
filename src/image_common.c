#include "./image_common.h"

void bitstream_init(struct bitStream *bs, uint8_t *data, size_t length) {
    bs->data = data;
    bs->bitpos = 0;
    bs->length = length;
}

int bitstream_read(struct bitStream *bs, int n, uint32_t *out) {
    if (n <= 0 || n > 32) return -1;  // invalid number of bits

    uint32_t result = 0;
    for (int i = 0; i < n; i++) {
        size_t byte_pos = bs->bitpos / 8;
        size_t bit_in_byte = bs->bitpos % 8;

        if (byte_pos >= bs->length) {
            return -1; // end of stream
        }

        uint8_t bit = (bs->data[byte_pos] >> bit_in_byte) & 1;
        result |= (bit << i); // LSB first
        bs->bitpos++;
    }
    *out = result;
    return 0; // success
}

int bitstream_peek(struct bitStream *bs, int n, uint32_t *out) {
    size_t original_pos = bs->bitpos;
    int res = bitstream_read(bs, n, out);
    bs->bitpos = original_pos;
    return res;
}

void bitstream_align_byte(struct bitStream *bs) {
    bs->bitpos = (bs->bitpos + 7) & ~7;
}

