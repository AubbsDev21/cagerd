#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
 
#include "oci_config.h"


typedef struct
{
    char *bundle_path;
    char *pid_file;
    char *container_id;
}runtime_opts_t;

//This zeros out values 
static void runtime_opts_init(runtime_opts_t *opts)
{
    opts->bundle_path   = NULL;
    opts->pid_file      = NULL;
    opts->container_id  = NULL;
}

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s run [OPTIONS] CONTAINER_ID\n\n", prog);
    fprintf(stderr, "OPTIONS:\n");
    fprintf(stderr, "  --bundle PATH      Path to container bundle (required)\n");
    fprintf(stderr, "  --pid-file FILE    Write container PID to file\n");
    fprintf(stderr, "\nExample (from containerd):\n");
    fprintf(stderr, "  %s run --bundle /run/containers/myapp myapp-123\n", prog);

}

static int parse_args(int argc, char *argv[], runtime_opts_t *opts) {
    /* Setting up options for cli args*/
    static struct option long_options[] = {
        {"bundle", required_argument, 0, 'b'},
        {"pid-file", required_argument, 0, 'p'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    
    memset(opts, 0, sizeof(runtime_opts_t));

    optind = 2;

    while ((opt = getopt_long(argc, argv, "b:p", long_options, &option_index)) != -1) {
        switch (opt)
        {
        case 'b':
            opts->bundle_path = optarg;
            break;

        case 'p':
            opts->pid_file = optarg;
            break;   
        default:
            fprintf(stderr, "Unknown option\n");
            return -1;
        }
    }

    /* Container ID is positional arg after options */
    if (optind < argc) {
        opts->container_id = argv[optind];
    }

    /* Validate required arguments */
    if (!opts->bundle_path) {
        fprintf(stderr, "Error: --bundle is required\n");
        return -1;
    }
    
    if (!opts->container_id) {
        fprintf(stderr, "Error: CONTAINER_ID is required\n");
        return -1;
    }
      return 0;
}

int main(int argc, char *argv[])
{
    /* TODO: parse CLI flags (--bundle, --log-level, etc.) */
    /* ./cagerd run --bundle /path/to/bundle container-id */
    /*/path/to/bundle/config.json  ← The JSON file*/
    //setting var called opts = runtime_opts_t
    runtime_opts_t opts;
    //zero out var
    runtime_opts_init(&opts);

  //Wrong command use usage
    if (argc < 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    /*Checking the subcommand is "run" */
    if (strcmp(argv[1], "run") != 0) {
        usage(argv[0]);

        return EXIT_FAILURE;
    }

    printf("[main] cagerd - OCI Container Runtime\n");
    printf("[main] Parsing arguments...\n");

    /* Prasing the CLI arguements */
    if(parse_args(argc, argv, &opts) < 0) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    printf("[main] Container ID: %s\n", opts.container_id);
    printf("[main] Bundle path: %s\n", opts.bundle_path);





    /* TODO: load config.json from bundle dir */
    /* TODO: set up container environment */
    /* TODO: exec container process */

    return EXIT_SUCCESS;
}