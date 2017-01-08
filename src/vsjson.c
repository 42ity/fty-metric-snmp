/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <tomas@halman.net> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Tomas Halman
 * ----------------------------------------------------------------------------
 */

#include "vsjson.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

struct _vsjson_t {
    int state;
    const char *cursor;
    char *text;
    char *token;
    int tokensize;
};

vsjson_t *vsjson_new (const char *json)
{
    if (!json) return NULL;
    vsjson_t *self = (vsjson_t *) malloc (sizeof (vsjson_t));
    if (!self) return NULL;
    
    memset(self, 0, sizeof(vsjson_t));
    self->text = strdup (json);
    return self;
}

const char *_vsjson_set_token (vsjson_t *self, const char *ptr, size_t len)
{
    if (!ptr || !self) return NULL;
    
    if (!len) len = strlen (ptr);
    if (self->tokensize > len + 1) {
        // fits in
        strncpy (self->token, ptr, len);
        self->token[len] = 0;
        return self->token;
    }
    if (self->token) {
        free (self->token);
        self->token = NULL;
        self->tokensize = 0;
    }
    self->token = (char *) malloc (len + 1);
    if (!self->token) return NULL;
    strncpy (self->token, ptr, len);
    self->token[len] = 0;
    self->tokensize = len+1;
    return self->token;
}

const char* _vsjson_seek_to_next_token(vsjson_t *self)
{
    if (!self) return NULL;
    
    while (true) {
        if (self->cursor == NULL) return NULL;
        if (! isspace (self->cursor[0])) return self->cursor;
        self->cursor++;
    }
}

const char* _vsjson_find_next_token(vsjson_t *self, const char *start)
{
    if (!self) return NULL;
    
    const char *p = start;
    if (!start) p = self->text;
    while (true) {
        if (*p == 0) return NULL;
        if (!isspace(*p)) return p;
        p++;
    }
}

const char* _vsjson_find_string_end(vsjson_t *self, const char *start)
{
    if (!self || !start) return NULL;

    const char *p = start;
    if (*p != '"') return NULL;
    ++p;
    while (true) {
        switch(*p) {
        case 0:
            return NULL;
        case '\\':
            ++p;
            if (*p == 0) return NULL;
            break;
        case '"':
                return ++p;
        }
        ++p;
    }
}

const char* _vsjson_find_number_end(vsjson_t *self, const char *start)
{
    if (!self || !start) return NULL;

    const char *p = start;
    if (!(isdigit (*p) || *p == '-' || *p  == '+')) return NULL;
    ++p;
    while (true) {
        if (*p == 0) return NULL;
        if(isdigit (*p) || *p == '.' || *p == 'e' || *p == 'E' || *p == '-' || *p == '+') {
            ++p;
        } else {
            return p;
        }
    }
}

const char* _vsjson_find_keyword_end(vsjson_t *self, const char *start)
{
    if (!self || !start) return NULL;

    const char *p = start;
    if (!isalpha (*p)) return NULL;
    ++p;
    while (true) {
        if (*p == 0) return NULL;
        if(isalpha (*p)) {
            ++p;
        } else {
            return p;
        }
    }
}

const char* _vsjson_find_token_end(vsjson_t *self, const char *start)
{
    if (!self || !start) return NULL;

    const char *p = start;
    if (strchr ("{}[]:,",*p)) {
        return ++p;
    }
    if (*p == '"') {
        return _vsjson_find_string_end (self, p);
    }
    if (strchr ("+-0123456789", *p)) {
        return _vsjson_find_number_end (self, p);
    }
    if (isalpha (*p)) {
        return _vsjson_find_keyword_end (self, p);
    }
    return NULL;
}

const char* vsjson_first_token (vsjson_t *self)
{
    if (!self) return NULL;
    self->cursor = _vsjson_find_next_token (self, NULL);
    if (!self->cursor) return NULL;
    const char *p = _vsjson_find_token_end (self, self->cursor);
    if (p) {
        _vsjson_set_token (self, self->cursor, p - self->cursor);
        self->cursor = p;
        return self->token;
    }
    return NULL;
}

const char* vsjson_next_token (vsjson_t *self)
{
    if (!self) return NULL;
    self->cursor = _vsjson_find_next_token (self, self->cursor);
    if (!self->cursor) return NULL;
    const char *p = _vsjson_find_token_end (self, self->cursor);
    if (p) {
        _vsjson_set_token (self, self->cursor, p - self->cursor);
        self->cursor = p;
        return self->token;
    }
    return NULL;
}

void vsjson_destroy (vsjson_t **self_p)
{
    if (!self_p) return;
    if (!*self_p) return;
    vsjson_t *self = *self_p;
    if (self->text) free (self->text);
    if (self->token) free (self->token);
    free (self);
    *self_p = NULL;
}

int _vsjson_walk_array (vsjson_t *self, const char *prefix, vsjson_callback_t *func, void *data);

int _vsjson_walk_object (vsjson_t *self, const char *prefix, vsjson_callback_t *func, void *data)
{
    int result = 0;
    char *locator = NULL;
    char *key;

    const char *token = vsjson_next_token (self);
    while (token) {
        // token should be key or }
        switch (token[0]) {
        case '}':
            goto cleanup;
        case '"':
            // TODO
            key = vsjson_decode_string (token);
            token = vsjson_next_token (self);
            if (strcmp (token, ":") != 0) {
                result = -1;
                goto cleanup;
            }
            token = vsjson_next_token (self);
            if (!token) {
                result = -1;
                goto cleanup;
            }
            size_t s = strlen (prefix) + strlen (key) + 2;
            locator = (char *)malloc (s);
            if (!locator) {
                result = -2;
                goto cleanup;
            }
            snprintf(locator, s, "%s%c%s", prefix, VSJSON_SEPARATOR, key);
            switch (token[0]) {
            case '{':
                result = _vsjson_walk_object (self, locator, func, data);
                if (result != 0) goto cleanup;
                break;
            case '[':
                result = _vsjson_walk_array (self, locator, func, data);
                if (result != 0) goto cleanup;
                break;
            case ':':
            case ',':
            case '}':
            case ']':
                result = -1;
                goto cleanup;
            default:
                // this is the value
                result = func (&locator[1], token, data);
                if (result != 0) goto cleanup;
                break;
            }
            free (locator);
            locator = NULL;
            free (key);
            key = NULL;
            break;
        default:
            // this is wrong
            result = -1;
            goto cleanup;
        }
        token = vsjson_next_token (self);
        // now the token can be only '}' or ','
        if (!token) {
            result = -1;
            goto cleanup;
        }
        switch (token[0]) {
        case ',':
            token = vsjson_next_token (self);
            break;
        case '}':
            break;
        default:
            result = -1;
            goto cleanup;
        }
    }
 cleanup:
    if (locator) free (locator);
    if (key) free (key);
    return result;
}

int _vsjson_walk_array (vsjson_t *self, const char *prefix, vsjson_callback_t *func, void *data)
{
    int index = 0;
    int result = 0;
    char *locator = NULL;

    const char *token = vsjson_next_token (self);
    while (token) {
        size_t s = strlen (prefix) + 1 + sizeof (index)*3 + 1;
        locator = (char *) malloc (s);
        if (!locator) {
            result = -2;
            goto cleanup;
        }
        snprintf(locator, s, "%s%c%i", prefix, VSJSON_SEPARATOR, index);
        // token should be value or ]
        switch (token[0]) {
        case ']':
            goto cleanup;
        case ':':
        case ',':
        case '}':
            result = -1;
            goto cleanup;
        case '{':
            result = _vsjson_walk_object (self, locator, func, data);
            if (result != 0) goto cleanup;
            break;
        case '[':
            result = _vsjson_walk_array (self, locator, func, data);
            if (result != 0) goto cleanup;
            break;
        default:
            result = func (&locator[1], token, data);
            if (result != 0) goto cleanup;
            break;
        }
        free (locator);
        locator = NULL;

        token = vsjson_next_token (self);
        // now the token can be only ']' or ','
        if (!token) {
            result = -1;
            goto cleanup;
        }
        switch (token[0]) {
        case ',':
            token = vsjson_next_token (self);
            ++index;
            break;
        case ']':
            break;
        default:
            result = -1;
            goto cleanup;
        }
    }
 cleanup:
    if (locator) free (locator);
    return result;
}

int vsjson_walk_trough (vsjson_t *self, vsjson_callback_t *func, void *data)
{
    if (!self || !func) return -1;

    int result = 0;
    
    const char *token = vsjson_first_token (self);
    if (token) {
        switch (token[0]) {
        case '{':
            result = _vsjson_walk_object (self, "", func, data);
            break;
        case '[':
            result = _vsjson_walk_array (self, "", func, data);
            break;
        default:
            // this is bad
            result = -1;
            break;
        }
    }
    if (result == 0) {
        token = vsjson_next_token (self);
        if (token) result = -1;
    }
    return result;
}

char *vsjson_decode_string (const char *string)
{
    if (!string) return NULL;

    char *decoded = (char *) malloc (strlen (string));
    if (!decoded) return NULL;

    memset (decoded, 0, strlen (string));
    const char *src = string;
    char *dst = decoded;
    
    if (string[0] != '"' || string[strlen (string)-1] != '"') {
        // no quotes, this is not json string
        return NULL;
    }
    ++src;
    while(*src) {
        switch (*src) {
        case '\\':
            ++src;
            switch (*src) {
            case '\\':
            case '/':
            case '"':
                *dst = *src;
                ++dst;
            case 'b':
                *dst = '\b';
                ++dst;
                break;
            case 'f':
                *dst = '\f';
                ++dst;
                break;
            case 'n':
                *dst = '\n';
                ++dst;
                break;
            case 'r':
                *dst = '\r';
                ++dst;
                break;
            case 't':
                *dst = '\t';
                ++dst;
                break;
            //TODO \uXXXX
            }
            break;
        default:
            *dst = *src;
            ++dst;
        }
        ++src;
    }
    --dst;
    *dst = 0;
    return decoded;
}


char *vsjson_encode_string (const char *string)
{
    if (!string) return NULL;

    int capacity = strlen (string) + 15;
    int index = 1;
    const char *p = string;

    char * encoded = (char *) malloc (capacity);
    if (!encoded) return NULL;
    memset (encoded, 0, capacity);
    encoded[0] = '"';
    while (*p) {
        switch (*p) {
        case '"':
        case '\\':
        case '/':
            encoded[index++] = '\\';
            encoded[index++] = *p;
            break;
        case '\b':
            encoded[index++] = '\\';
            encoded[index++] = 'b';
            break;
        case '\f':
            encoded[index++] = '\\';
            encoded[index++] = 'f';
            break;
        case '\n':
            encoded[index++] = '\\';
            encoded[index++] = 'n';
            break;
        case '\r':
            encoded[index++] = '\\';
            encoded[index++] = 'r';
            break;
        case '\t':
            encoded[index++] = '\\';
            encoded[index++] = 't';
            break;
        default:
            encoded[index++] = *p;
            break;
        //TODO \uXXXX
        }
        p++;
        if (capacity - index < 10) {
            int add = strlen (p) + 15;
            char *ne = (char *) realloc (encoded, capacity + add);
            if (ne) {
                encoded = ne;
                memset (&encoded[capacity], 0, add);
                capacity += add;
            }else {
                free (encoded);
                return NULL;
            }
        }
    }
    encoded [index] = '"';
    return encoded;
}

int vsjson_parse (const char *json, vsjson_callback_t *func, void *data)
{
    if (!json || !func) return -1;
    vsjson_t *v = vsjson_new (json);
    int r = vsjson_walk_trough (v, func, data);
    vsjson_destroy (&v);
    return r;
}
