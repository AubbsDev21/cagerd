#ifndef OCI_CONFIG_H
#define OCI_CONFIG_H

#include <stdlib.h>
#include <limits.h>

/**
 * Minimal OCI Runtime Config - only fields we actually need
 * 
 * Everything else is ignored for now. Build in this order:
 * 1. Parse config ← you are here
 * 2. Setup namespaces
 * 3. Setup rootfs
 * 4. Setup cgroups
 * 5. exec() process
 */

typedef struct {
    /* process.args - what to execute */
    char **args;
    int args_len;
    
    /* process.env - environment variables */
    char **env;
    int env_len;
    
    /* process.cwd - working directory */
    char *cwd;
    
    /* root.path - root filesystem path */
    char *rootfs;
    
    /* process.user.uid / process.user.gid */
    int uid;
    int gid;
    
    /* container id (from CLI args, not config.json) */
    char *container_id;
} oci_config_t;

/**
 * oci_config_load - Load config.json from bundle path
 * 
 * Args:
 *   bundle_path: Path to container bundle (contains config.json)
 *   config: Pointer to oci_config_t to fill
 * 
 * Returns: 0 on success, -1 on error
 */
int oci_config_load(const char *bundle_path, oci_config_t *config);

/**
 * oci_config_free - Free allocated config memory
 */
void oci_config_free(oci_config_t *config);

/**
 * oci_config_print - Debug: print loaded config
 */
void oci_config_print(const oci_config_t *config);

#endif /* OCI_CONFIG_H */

