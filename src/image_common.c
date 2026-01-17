#include <stdio.h>
#include "./image_common.h"

// Reverse the lowest n bits of x
uint32_t reverse_bits(uint32_t x, int n) {
    uint32_t r = 0;
    for (int i = 0; i < n; i++) {
        r <<= 1;
        r |= (x >> i) & 1;
    }
    return r;
}

void print_binary(uint32_t value, int bits) {
    for (int i = bits - 1; i >= 0; i--) {
        printf("%c", (value & (1 << i)) ? '1' : '0');
    }
    putchar('\n');
}

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

// Write n bits from 'in' to the bitstream (LSB first)
int bitstream_write(struct bitStream *bs, int n, uint32_t in) {
    if (n <= 0 || n > 32) return -1; // invalid number of bits

    for (int i = 0; i < n; i++) {
        size_t byte_pos = bs->bitpos / 8;
        size_t bit_in_byte = bs->bitpos % 8;

        if (byte_pos >= bs->length) {
            return -1; // end of buffer
        }

        uint8_t bit = (in >> i) & 1; // LSB first
        bs->data[byte_pos] &= ~(1 << bit_in_byte); // clear target bit
        bs->data[byte_pos] |= bit << bit_in_byte; // set target bit

        bs->bitpos++;
    }
    return 0; // success
}

int bitstream_flush(struct bitStream *bs) {
    size_t byte_pos = bs->bitpos / 8;
    size_t bit_in_byte = bs->bitpos % 8;
    if (bit_in_byte == 0) {
        return 0;
    }
    uint8_t padding_amt = 8 - bit_in_byte;
    bs->data[byte_pos] &= (1 << bit_in_byte) - 1; // clear bits after current pos to the byte end
    bs->bitpos += padding_amt;
    return 0;
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

void bitstream_print(struct bitStream *bs) {
    size_t total_bytes = (bs->bitpos + 7) / 8; // how many bytes are actually written

    printf("Bitstream (%zu bits, %zu bytes):\n", bs->bitpos, total_bytes);

    for (size_t i = 0; i < total_bytes; i++) {
        uint8_t byte = bs->data[i];
        // Print hex
        printf("0x%02X  ", byte);

        // Print bits in LSB-first order
        for (int b = 0; b < 8; b++) {
            printf("%d", (byte >> b) & 1);
        }
        printf("\n");
    }
}

int bitstream_get_size(struct bitStream *bs) {
    return (bs->bitpos + 7)/8;
}
