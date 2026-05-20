#ifndef CJSON_H
#define CJSON_H

/* cJSON — vendored, dependency-free JSON parser */
/* https://github.com/DaveGamble/cJSON  (MIT License) */

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* cJSON value types */
#define cJSON_Invalid  (0)
#define cJSON_False    (1 << 0)
#define cJSON_True     (1 << 1)
#define cJSON_NULL     (1 << 2)
#define cJSON_Number   (1 << 3)
#define cJSON_String   (1 << 4)
#define cJSON_Array    (1 << 5)
#define cJSON_Object   (1 << 6)
#define cJSON_Raw      (1 << 7)

typedef struct cJSON {
    struct cJSON *next;
    struct cJSON *prev;
    struct cJSON *child;
    int           type;
    char         *valuestring;
    int           valueint;     /* deprecated, use valuedouble */
    double        valuedouble;
    char         *string;
} cJSON;

/* TODO: declare cJSON API functions (parse, print, get, free, …) */

#ifdef __cplusplus
}
#endif

#endif /* CJSON_H */