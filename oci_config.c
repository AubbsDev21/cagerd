#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cjson/cJSON.h>
 
#include "oci_config.h"

 



/**
 * oci_config_free - Free all memory owned by a config struct
 * Invoked by: caller of oci_config_load() when done with the config
 * Parameters passed in:
 *   cfg (oci_config_t *) - the config struct to free
 * Returns: nothing
 */
void oci_config_free(oci_config_t *cfg)
{
    /* TODO: free all nested allocations, then free cfg */
    (void)cfg;
}

/**
 * read_file - Read an entire file into a heap-allocated buffer
 * Invoked by: oci_config_load() to load config.json into memory
 * Parameters passed in:
 *   path (const char *) - path to the file to open and read
 *   size (size_t *)     - gets written with the number of bytes read
 * Returns: pointer to the buffer holding the file contents, or NULL on error
 *          caller is responsible for calling free() on the returned pointer
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
     fprintf(stdout, "[read_file] heap size within buffer: %zu bytes\n", *size);

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
 * parse_string_array - Convert cJSON array to char** array
 * Why: cJSON stores data in its own tree structure we can't pass that directly
 *      to execv() to launch a process. This converts it into a plain C string array
 *      that the OS understands.
 * Invoked by: oci_config_load() when extracting process.args
 * Parameters passed in:
 *   array (cJSON *)  - the JSON array to read from
 *                      e.g. cJSON representing: ["bin/sh", "-c", "echo hello"]
 *   len   (int *)    - gets written with the number of strings found
 * Returns: heap-allocated array of char* (NULL terminated), or NULL on error
 *          e.g. { "bin/sh", "-c", "echo hello", NULL }  with len = 3
 */
static char **parse_string_array(cJSON *array, int *len) {
    /* Bail if we got nothing or it's not actually a JSON array */
    if (!array || !cJSON_IsArray(array)) {
        *len = 0;
        return NULL;
    }

    /* Count how many items are in the array and store it in len */
    *len = cJSON_GetArraySize(array);
    if (*len == 0) {
        return NULL;
    }

    /* Allocate a list of string pointers, +1 for a NULL sentinel at the end */
    char **result = malloc((*len + 1) * sizeof(char *));
    if (!result) {
        perror("malloc");
        return NULL;
    }

    /* Walk each item, make sure it's a string, then copy it into the list */
    for (int i = 0; i < *len; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        if (!cJSON_IsString(item)) {
            fprintf(stderr, "oci_config: Array item %d is not a string\n", i);
            free(result);
            return NULL;
        }
        result[i] = strdup(item->valuestring);
    }

    /* NULL terminate the list so callers can loop until they hit NULL */
    result[*len] = NULL;

    fprintf(stdout, "[parse_string_array] args_len: %d\n", *len);
    return result;


}



/**
 * oci_config_load - Read and parse config.json from an OCI bundle directory
 * Invoked by: main() when the runtime starts a container
 * Parameters passed in:
 *   bundle_path (const char *)  - path to the bundle folder (e.g. /path/to/bundle)
 *   config      (oci_config_t *)- empty struct that gets filled with the parsed values
 * Returns: 0 on success, -1 on any error (missing file, bad JSON, missing fields)
 */

 int oci_config_load(const char *bundle_path, oci_config_t *config) {
    //Tells us the Maximum length a file path can be on this OS
    char config_path[PATH_MAX];
    size_t file_size;
    char *json_text;
    struct stat st;
    cJSON *root, *process, *root_obj, *user, *args, *env, *cwd, *rootfs, *uid, *gid;

    /*Error Handling: if paramters do not exist*/
    if (!bundle_path || !config) {
        fprintf(stderr, "oci_config_load: invalid arguments\n");
        return -1;
    }

    /*Init config by zero values in config*/
    memset(config, 0, sizeof(oci_config_t));

    /*Building the config.json path */
    snprintf(config_path, sizeof(config_path), "%s/config.json", bundle_path);

    printf("[config] Loading from: %s\n", config_path);
    
    /* Checking if directory given is an actually directory in the filesystem*/
    if(stat(bundle_path, &st) !=0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "[oci_config_load] bundle_path \"%s\" is not a valid directory: %s\n",
        bundle_path, strerror(errno));
        return -1;
    }

    /*Reading the config.json file*/
    json_text = read_file(config_path, &file_size);
    if (!json_text) {
        fprintf(stderr, "oci_config: Failed to read %s\n", config_path);
        return -1;
    }

    /*Converts it into a traversable tree then loads the json text into a cJSON type then freeing the heap memory*/
    root = cJSON_Parse(json_text);
    free(json_text);

    if (!root) {
        fprintf(stderr, "[oci_config]: Failed to parse JSON\n");
        return -1;
    }

    fprintf(stdout, "[OCI_CONFIG] Json text loaded into a cJson type, memory freed\n");

    fprintf(stdout, "[OCI_CONFIG] Extracting process fields from JSON\n");


    /* Extracting the "process" arguments from the Json data*/
    process = cJSON_GetObjectItem(root, "process");
    if (!process) {
        fprintf(stderr, "[OCI_CONFIG] Missing 'process' object\n");
        cJSON_Delete(root);
        return -1;
    }
    fprintf(stdout, "[OCI_CONFIG] process: %s\n", cJSON_PrintUnformatted(process));


    /*Retriving the "args" key from the process level key*/
    args = cJSON_GetObjectItem(process, "args");
    if (!args) {
        fprintf(stderr, "[OCI_CONFIG] Missing 'args' object\n");
        cJSON_Delete(root);
        return -1;
    }
    fprintf(stdout, "[OCI_CONFIG] args: %s\n", cJSON_PrintUnformatted(args));



    config->args = parse_string_array(args, &config->args_len);
    fprintf(stdout, "[Debuging] Ending of oci_cofig.c\n");
    if (!config->args) {
        fprintf(stderr, "[OCI_CONFIG] Invalid or empty 'process.args'\n");
        cJSON_Delete(root);
        return -1;
    }
    fprintf(stdout, "[config] process.args: ");
    for (int i = 0; i < config->args_len; i++ ){
        printf("%s ", config->args[i]);
    }
    printf("\n");

    /* Extract process.env from config.json json data*/
    env = cJSON_GetObjectItem(process, "env");
    /* Converting "env" json array into a **char array*/
    if (env) {
        config->env = parse_string_array(env, &config->env_len);
        printf("[config] process.env: %d variables\n", config->env_len);
    }

    /* Extracting process.cwd from cJSON heap buffer */
    cwd = cJSON_GetObjectItem(process, "cwd");
    if (!cwd) {
        fprintf(stderr, "[OCI_CONFIG] Missing 'process.cwd' object\n");
        cJSON_Delete(root);
        return -1;
    }
    fprintf(stdout, "[OCI_CONFIG] cwd: %s\n", cJSON_PrintUnformatted(cwd));


    /* Extracting process.user.uid/gid from cJSON heap buffer */
    user = cJSON_GetObjectItem(process, "user");
    if (!user){ 
       fprintf(stderr, "[OCI_CONFIG] Missing 'process.cwd' object\n");
       cJSON_Delete(root);
       return -1;
    }

    if (user) {
        uid = cJSON_GetObjectItem(user, "uid");
        gid = cJSON_GetObjectItem(user, "gid");

        if (uid && cJSON_IsNumber(uid) && gid && cJSON_IsNumber(gid)) {
            fprintf(stdout, "[OCI_CONFIG] uid: %s\n", cJSON_PrintUnformatted(uid));
            fprintf(stdout, "[OCI_CONFIG] gid: %s\n", cJSON_PrintUnformatted(uid));

        }
    }

    /* Extract root.path (required) */
    root_obj = cJSON_GetObjectItem(root, "root");
    if (!root_obj) {
        fprintf(stderr, "oci_config: Missing 'root' object\n");
        cJSON_Delete(root);
        return -1;
    }
    fprintf(stdout, "[OCI_CONFIG] root object: %s\n", cJSON_PrintUnformatted(root_obj));


    rootfs = cJSON_GetObjectItem(root_obj, "path");
    if (!rootfs) {
        fprintf(stderr, "oci_config: Missing 'root.path'\n");
        cJSON_Delete(root);
        return -1;
    }
    fprintf(stdout, "[OCI_CONFIG] Root Path: %s\n", cJSON_PrintUnformatted(uid));


    cJSON_Delete(root);
    
    printf("[config] Configuration loaded successfully\n");
    return 0;



    return 0;
    


 }
