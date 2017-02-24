/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <tomas@halman.net> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Tomas Halman
 * ----------------------------------------------------------------------------
 */

#ifndef __VSJSON_H
#define __VSJSON_H

#ifdef __cplusplus
extern "C" {
#endif

#define VSJSON_SEPARATOR '/'

#ifndef VSJSON_T_DEFINED
typedef struct _vsjson_t vsjson_t;
#define VSJSON_T_DEFINED
#endif

typedef int (vsjson_callback_t)(const char *locator, const char *value, void *data);

// minimalized json parser class
// returns new parser object
// parameter is json string
// call vsjson_destroy to free the parser
vsjson_t *vsjson_new (const char *json);

// destructor of json parser
void vsjson_destroy (vsjson_t **self_p);

// get first json token, usually "[" or "{"
// tokens are [ ] { } , : string (quote included)
// number or keyword like null
const char* vsjson_first_token (vsjson_t *self);

// get next json token
// walk through json like this:
//     vsjson *parser = vsjson_new (jsonString);
//     const char *token = vsjson_first_token (parser);
//     while (token) {
//        printf ("%s ", token);
//        token = vsjson_next_token (parser);
//     }
//     printf ("\n");
//     vsjson_destroy (&parser);
const char* vsjson_next_token (vsjson_t *self);

int vsjson_walk_trough (vsjson_t *self, vsjson_callback_t *func, void *data);

int vsjson_parse (const char *json, vsjson_callback_t *func, void *data);

char *vsjson_decode_string (const char *string);

char *vsjson_encode_string (const char *string);

#ifdef __cplusplus
}
#endif

#endif // __VSJSON_H
