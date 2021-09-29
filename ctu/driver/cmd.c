#include "cmd.h"

#include "ctu/util/util.h"
#include "ctu/util/str.h"

#include <stdlib.h>

static const char *VERSION = "0.0.1";
static const char *NAME = "driver";

static void print_help(void) {
    printf("Cthulhu Compiler Collection\n");
    printf("Usage: %s [options...] [sources...]\n", NAME);
    printf("Options:\n");
    printf("\t -h, --help: Print this help message\n");
    printf("\t -v, --version: Print version information\n");
    printf("\t -src, --source: Override file extension based compiler choice\n");
    printf("\t -gen, --generator: Override default backend code generator\n");
    printf("\t -V, --verbose: Enable verbose logging\n");

    exit(0);
}

static void print_version(void) {
    printf("Cthulhu Compiler Collection\n");
    printf("Version: %s\n", VERSION);
    printf("Frontends:\n");
    printf("* Cthulhu Version: %s\n", FRONTEND_CTU.version);
    printf("* PL/0 Version: %s\n", FRONTEND_PL0.version);
    printf("* C Version: %s\n", FRONTEND_C11.version);
    printf("Backends:\n");
    printf("* C99 Version: %s\n", BACKEND_C99.version);
    printf("* GCCJIT Version: %s\n", BACKEND_GCCJIT.version);
    printf("* LLVM Version: %s\n", BACKEND_LLVM.version);

    exit(0);
}

#define MATCH(arg, a, b) (startswith(arg, a) || startswith(arg, b))
#define NEXT(idx, argc, argv) (idx + 1 >= argc ? NULL : argv[idx + 1])

static int parse_arg(settings_t *settings, int index, int argc, char **argv) {
    const char *arg = argv[index];
    
    if (!startswith(arg, "-")) {
        file_t *fp = ctu_open(arg, "rb");

        if (fp == NULL) {
            report(settings->reports, ERROR, NULL, "failed to open file: %s", arg);
        } else {
            vector_push(&settings->sources, fp);
        }
    } else if (MATCH(arg, "-h", "--help")) {
        print_help();
    } else if (MATCH(arg, "-v", "--version")) {
        print_version();
    } else if (MATCH(arg, "-src", "--source")) {
        if (settings->frontend != NULL) {
            report(settings->reports, ERROR, NULL, "source already specified");
        }

        settings->frontend = select_frontend(settings->reports, NEXT(index, argc, argv));
        return 2;
    } else if (MATCH(arg, "-gen", "--generator")) {
        if (settings->backend != NULL) {
            report(settings->reports, ERROR, NULL, "generator already specified");
        }

        settings->backend = select_backend(settings->reports, NEXT(index, argc, argv));
        return 2;
    } else if (MATCH(arg, "-V", "--verbose")) {
        settings->verbose = true;
    } else {
        report(settings->reports, WARNING, NULL, "unknown argument %s", arg);
    }

    return 1;
}

settings_t parse_args(reports_t *reports, int argc, char **argv) {
    NAME = argv[0];

    settings_t settings = { 
        .frontend = NULL, 
        .backend = NULL,
        .sources = vector_new(0),
        .reports = reports,
        .verbose = false
    };

    if (argc == 1) {
        print_help();
    }

    for (int i = 1; i < argc;) {
        i += parse_arg(&settings, i, argc, argv);
    }

    return settings;
}

