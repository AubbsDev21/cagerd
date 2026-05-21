#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
 
#include <cjson/cJSON.h>
 
#include "oci_config.h"

 


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
/**
 * read_file - Read entire file into allocated buffer
 */

static char *read_file(const char *path, size_t *size) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "[read_file] fopen failed for \"%s\": %s\n", path, strerror(errno));
        return NULL;
    }
    fprintf(stdout, "[read_file] fopen succeeded for \"%s\"\n", path);

    /* Getting the size of file*/
     fseek(fp, 0, SEEK_END);
     *size = ftell(fp);
     fprintf(stdout, "[read_file] file size: %zu bytes\n", *size);
     fseek(fp, 0, SEEK_SET);

     /*Allocates space memory on the heap for file*/
     char *buf = malloc(*size + 1);
     fprintf(stdout, "[read_file] heap size within buffer: %zu bytes\n", *buf);

     if (!buf) {
        perror("malloc");
        fclose(fp);
        return NULL;
     }

    /*Checking if buffer has the correct size for file*/
     if (fread(buf, 1, *size, fp) != *size) {
        perror("fread");
        free(buf);
        fclose(fp);
        return NULL;

     }
     /* Loads file into the buffer/heap */
     buf[*size] = '\0';
     fclose(fp);

     return buf;
    

}

/**
 * oci_config_load - Parse config.json from bundle
 * 
 * Standard containerd call:
 *   cagerd run --bundle /path/to/bundle container-id
 * 
 * config.json is at:
 *   /path/to/bundle/config.json
 */

 int oci_config_load(const char *bundle_path, oci_config_t *config) {
    //Tells us the Maximum length a file path can be on this OS
    char config_path[PATH_MAX];
    size_t file_size;
    char *json_text;
    cJSON *root, *process, *root_obj, *user;

    /*Error Handling: if paramters do not exist*/
    if (!bundle_path || !config) {
        fprintf(stderr, "oci_config_load: invalid arguments\n");
        return -1;
    }

    /*Init config by zero values in config*/
    memset(config, 0, sizeof(oci_config_t));
    config->uid = 0;
    config->gid = 0;
    config->cwd = strdup("/");

    /*Building the config.json path */

    snprintf(config_path, sizeof(config_path), "%s/config.json", bundle_path);

    printf("[config] Loading from: %s\n", config_path);

    /*Reading the config.json file*/
    json_text = read_file(config_path, &file_size);
    if (!json_text) {
        fprintf(stderr, "oci_config: Failed to read %s\n", config_path);
        return -1;
    }
    return 0;
    


 }
