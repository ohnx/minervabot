#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hashmap.h"

/* CRC8 */
#define CRC8_TOPBIT (1 << 7)
#define CRC8_POLYNOMIAL 0x9b

static unsigned char table8[256];
static int crc8_hinit = 0;

void crc8_init() {
    int dividend;
    unsigned char bit, remainder;

    crc8_hinit = 1;

    /* Compute the remainder of each possible dividend. */
    for (dividend = 0; dividend < 256; dividend++) {
        /* Start with the dividend followed by zeros. */
        remainder = dividend;

        /* Perform modulo-2 division, a bit at a time. */
        for (bit = 8; bit > 0; --bit) {
            /* Try to divide the current data bit. */
            if (remainder & CRC8_TOPBIT) {
                remainder = (remainder << 1) ^ CRC8_POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }

        /* Store the result into the table. */
        table8[dividend] = remainder;
    }
}

unsigned short crc8_block(const unsigned char *bytes, long byteLen) {
    long rpos = 0;
    unsigned char remainder = 0, data;

    if (!crc8_hinit) crc8_init();

    /* Divide the message by the polynomial, a byte at a time. */
    for (rpos = 0; rpos < byteLen; ++rpos) {
        data = bytes[rpos] ^ remainder;
        remainder = table8[data] ^ (remainder << 8);
    }

    /* The final remainder is the CRC. */
    return remainder & 0xff;
}

/* Hashmap */
hashmap *hashmap_new() {
    hashmap *ret;
    hashmap_entry_list *buckets;

    if (!crc8_hinit) crc8_init();

    /* clear initial values */
    ret = calloc(1, sizeof(hashmap));
    buckets = calloc(256, sizeof(hashmap_entry));

    if (ret == NULL || buckets == NULL) return NULL;

    ret->num_entry = 0;
    ret->bucketsize = 256;
    ret->buckets = buckets;
    return ret;
}

void hashmap_put(hashmap *in, const char *key, int value) {
    unsigned short hash;
    hashmap_entry_list *hel;
    hashmap_entry *he;

    /* get hash value */
    hash = crc8_block((const unsigned char *)key, (long)strlen(key));
    hel = &in->buckets[hash];

    /* reallocate memory if necessary */
    if (hel->vroom < ++(hel->vlen)) {
        hel->vroom = (hel->vroom == 0 ? 4 : hel->vroom*2);
        he = realloc(hel->values, hel->vroom * sizeof(hashmap_entry));

        if (he == NULL) {
            return; /* TODO: Throw exception */
        }
        
        hel->values = he;
    }

    /* jump to end of hashmap entry list */
    he = &hel->values[hel->vlen-1];

    he->key = strdup(key);
    he->value = value;

    /* success */
    in->num_entry++;
}

int hashmap_remove(hashmap *in, const char *key) {
    unsigned short hash;
    hashmap_entry_list *hel;
    hashmap_entry *he;
    int tmp;
    int i;

    /* get hash value */
    hash = crc8_block((const unsigned char *)key, (long)strlen(key));
    hel = &in->buckets[hash];

    /* loop through all the key/value pairs with that hash */
    for (i = 0; i < hel->vlen; i++) {
        he = &hel->values[i];
        if (!strcmp(key, he->key)) goto found;
    }
    return 0;

    /* success */
    found:
    /* temporarily store the value */
    tmp = he->value;
    /* free the key */
    free(he->key);
    /* copy over the values past this to over this */
    memcpy(hel->values + i, hel->values + i + 1, sizeof(hashmap_entry) * (hel->vlen - i));
    /* decrease the length */
    hel->vlen--;
    return tmp;
}

int hashmap_get(hashmap *in, const char *key) {
    unsigned short hash;
    hashmap_entry_list *hel;
    hashmap_entry *he;
    int i;

    /* get hash value */
    hash = crc8_block((const unsigned char *)key, (long)strlen(key));
    hel = &in->buckets[hash];

    /* loop through all the key/value pairs with that hash */
    for (i = 0; i < hel->vlen; i++) {
        he = &hel->values[i];
        if (!strcmp(key, he->key)) goto found; /* found exact key */
    }
    return 0;

    /* success */
    found:
    return he->value;
}

int hashmap_iterate(hashmap *in, hashmap_iterator iter, void *context) {
    hashmap_entry_list *hel;
    hashmap_entry *he;
    int i, j;
    
    /* loop through all the buckets */
    for (i = 0; i < in->bucketsize; i++) {
        hel = &in->buckets[i];

        /* loop through all the key/value pairs */
        for (j = 0; j < hel->vlen; j++) {
            he = &hel->values[j];

            /* call the function */
            if (iter(context, he->key, he->value))
                return -1;
        }
    }

    return 0;
}

void hashmap_empty(hashmap *in) {
    hashmap_entry_list *hel;
    hashmap_entry *he;
    int i, j;

    /* loop through all the buckets */
    for (i = 0; i < in->bucketsize; i++) {
        hel = &in->buckets[i];

        /* loop through all the key/value pairs */
        for (j = 0; j < hel->vlen; j++) {
            he = &hel->values[j];

            /* free the key */
            free(he->key);
        }

        /* reset the key/value pairs array */
        free(hel->values);
        hel->values = NULL;
        hel->vlen = 0;
        hel->vroom = 0;
    }
}

void hashmap_destroy(hashmap *in) {
    /* empty hashmap */
    hashmap_empty(in);

    /* clean up bucket storage */
    free(in->buckets);
    free(in);
}
