/* vim: set tabstop=8 shiftwidth=4 softtabstop=4 expandtab smarttab colorcolumn=80: */

#define _GNU_SOURCE
#include "json.h"
#include <string.h>

static json_t *
enc_json(const json_t *json)
{
    json_t *out = NULL;
    char *buf = NULL;

    buf = json_dumps(json, JSON_SORT_KEYS | JSON_COMPACT);
    if (!buf)
        return NULL;

    out = json_from_buf((uint8_t *) buf, strlen(buf));
    free(buf);
    return out;
}

static json_t *
dec_json(const json_t *json)
{
    uint8_t *buf = NULL;
    json_t *out = NULL;
    size_t len = 0;

    buf = json_to_buf(json, &len);
    if (!buf)
        return NULL;

    out = json_loads((char *) buf, 0, NULL);
    free(buf);
    return out;
}

json_t *
jose_jws_from_compact(const char *jws)
{
    size_t len[3] = { 0, 0, 0 };

    for (size_t c = 0, i = 0; jws[i]; i++) {
        if (jws[i] != '.')
            len[c]++;
        else if (++c > 2)
            return NULL;
    }

    if (len[0] == 0 || len[1] == 0 || len[2] == 0)
        return NULL;

    return json_pack("{s: s%, s: s%, s: s%}",
                     "payload", &jws[len[0] + 1], len[1],
                     "protected", jws, len[0],
                     "signature", &jws[len[0] + len[1] + 2], len[2]);
}

json_t *
jose_jws_from_payload(const uint8_t pay[], size_t len)
{
    return json_pack("{s: o}", "payload", json_from_buf(pay, len));
}

json_t *
jose_jws_from_payload_json(const json_t *pay)
{
    return json_pack("{s: o}", "payload", enc_json(pay));
}

char *
jose_jws_to_compact(const json_t *jws)
{
    const char *signature = NULL;
    const char *protected = NULL;
    const char *payload = NULL;
    const char *header = NULL;
    char *out = NULL;

    if (json_unpack((json_t *) jws, "{s: s, s: s, s: s, s? s}",
                    "payload", &payload,
                    "signature", &signature,
                    "protected", &protected,
                    "header", &header) == -1 &&
        json_unpack((json_t *) jws, "{s: s, s: [{s: s, s: s, s: s, s? s}!]}",
                    "payload", &payload,
                    "signatures",
                    "signature", &signature,
                    "protected", &protected,
                    "header", &header) == -1)
        return NULL;

    if (header)
        return NULL;

    asprintf(&out, "%s.%s.%s", protected, payload, signature);
    return out;
}

uint8_t *
jose_jws_to_payload(const json_t *jws, size_t *len)
{
    return json_to_buf(json_object_get(jws, "payload"), len);
}

json_t *
jose_jws_to_payload_json(const json_t *jws)
{
    uint8_t *pay = NULL;
    json_t *out = NULL;
    size_t len = 0;

    pay = jose_jws_to_payload(jws, &len);
    if (!pay)
        return NULL;

    out = json_loads((char *) pay, 0, NULL);
    free(pay);
    return out;
}

bool
jose_jws_sign(json_t *jws, const json_t *head, const json_t *prot,
              const json_t *jwks)
{
    const json_t *array = NULL;

    if (json_is_array(jwks))
        array = jwks;
    else if (json_is_array(json_object_get(jwks, "keys")))
        array = json_object_get(jwks, "keys");

    if (array) {
        for (size_t i = 0; i < json_array_size(array); i++) {
            const json_t *jwk = json_array_get(array, i);

            if (!jose_jws_sign(jws, head, prot, jwk))
                return false;
        }

        return true;
    }

#warning TODO
}

bool
jose_jws_verify(const json_t *jws, const json_t *jwks, bool all)
{
#warning TODO
    return false;
}
