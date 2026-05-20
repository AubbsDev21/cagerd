#include <stdlib.h>
#include "oci_config.h"
#include "cJSON.h"

oci_config_t *oci_config_load(const char *path)
{
    /* TODO: read file, parse JSON with cJSON, populate struct */
    (void)path;
    return NULL;
}

void oci_config_free(oci_config_t *cfg)
{
    /* TODO: free all nested allocations, then free cfg */
    (void)cfg;
}