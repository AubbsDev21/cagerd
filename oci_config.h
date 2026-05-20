#ifndef OCI_CONFIG_H
#define OCI_CONFIG_H

/* OCI Runtime Spec config.json struct definitions */

typedef struct {
    /* TODO: args, env, cwd, terminal */
} oci_process_t;

typedef struct {
    /* TODO: path, readonly */
} oci_root_t;

typedef struct {
    /* TODO: destination, type, source, options */
} oci_mount_t;

typedef struct {
    char            *oci_version;
    oci_process_t    process;
    oci_root_t       root;
    oci_mount_t     *mounts;
    int              mounts_count;
    /* TODO: hooks, linux, annotations */
} oci_config_t;

/* Load and parse an OCI config.json from disk */
oci_config_t *oci_config_load(const char *path);

/* Free all memory owned by an oci_config_t */
void oci_config_free(oci_config_t *cfg);

#endif /* OCI_CONFIG_H */