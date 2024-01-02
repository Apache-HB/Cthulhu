#include "argparse/argparse.h"
#include "std/colour.h"
#include "base/log.h"
#include "base/panic.h"
#include "core/macros.h"
#include "defaults/defaults.h"
#include "format/config.h"
#include "format/version.h"
#include "io/io.h"
#include "memory/memory.h"
#include "notify/notify.h"
#include "notify/text.h"
#include "os/os.h"
#include "stacktrace/stacktrace.h"

#include "config/config.h"
#include "std/str.h"
#include "std/vector.h"
#include <stdlib.h>

static const cfg_info_t kGroupInfo = {
    .name = "general",
    .brief = "General options",
};

static const char *const kHelpInfoShortArgs[] = {"h", "?", NULL};
static const char *const kHelpInfoLongArgs[] = {"help", NULL};

static const cfg_info_t kHelpInfo = {
    .name = "help",
    .brief = "Print this help message and exit",
    .short_args = kHelpInfoShortArgs,
    .long_args = kHelpInfoLongArgs,
};

static const char *const kVersionInfoShortArgs[] = {"v", NULL};
static const char *const kVersionInfoLongArgs[] = {"version", NULL};

static const cfg_info_t kVersionInfo = {
    .name = "version",
    .brief = "Print version information and exit",
    .short_args = kVersionInfoShortArgs,
    .long_args = kVersionInfoLongArgs,
};

static const char *const kArgparseUsageInfoShortArgs[] = {"u", NULL};
static const char *const kArgparseUsageInfoLongArgs[] = {"argparse-usage", NULL};

static const cfg_info_t kArgparseUsageInfo = {
    .name = "argparse-usage",
    .brief = "Print argparse usage information",
    .short_args = kArgparseUsageInfoShortArgs,
    .long_args = kArgparseUsageInfoLongArgs,
};

static const char *const kWindowsStyleInfoShortArgs[] = {"w", NULL};
static const char *const kWindowsStyleInfoLongArgs[] = {"windows-style", NULL};

static const cfg_info_t kWindowsStyleInfo = {
    .name = "windows-style",
    .brief = "Enable windows style help output\nprints /flags instead of -flags",
    .short_args = kWindowsStyleInfoShortArgs,
    .long_args = kWindowsStyleInfoLongArgs,
};

static const char *const kColourInfoShortArgs[] = {"c", NULL};
static const char *const kColourInfoLongArgs[] = {"colour", NULL};

static const cfg_info_t kColourInfo = {
    .name = "colour",
    .brief = "Enable colour output",
    .short_args = kColourInfoShortArgs,
    .long_args = kColourInfoLongArgs,
};

static const cfg_info_t kDebugGroupInfo = {
    .name = "debug",
    .brief = "Internal debugging options",
};

static const char *const kVerboseLoggingInfoShortArgs[] = {"V", NULL};
static const char *const kVerboseLoggingInfoLongArgs[] = {"verbose", NULL};

static const cfg_info_t kVerboseLoggingInfo = {
    .name = "verbose",
    .brief = "Enable verbose logging",
    .short_args = kVerboseLoggingInfoShortArgs,
    .long_args = kVerboseLoggingInfoLongArgs,
};

static const char *const kBacktraceInfoLongArgs[] = { "bt-complex", NULL };

static const cfg_info_t kBacktraceInfo = {
    .name = "backtrace",
    .brief = "Enable complex backtraces",
    .long_args = kBacktraceInfoLongArgs,
};

default_options_t get_default_options(config_t *group)
{
    CTASSERT(group != NULL);

    config_t *general = config_group(group, &kGroupInfo);

    cfg_bool_t help_initial = { .initial = false };
    cfg_field_t *help = config_bool(general, &kHelpInfo, help_initial);

    cfg_bool_t version_initial = { .initial = false };
    cfg_field_t *version = config_bool(general, &kVersionInfo, version_initial);

    cfg_bool_t argparse_usage_initial = { .initial = false };
    cfg_field_t *argparse_usage = config_bool(general, &kArgparseUsageInfo, argparse_usage_initial);

    cfg_bool_t windows_style_initial = { .initial = DISPLAY_WIN_STYLE };
    cfg_field_t *windows_style = config_bool(general, &kWindowsStyleInfo, windows_style_initial);

    cfg_bool_t colour_initial = { .initial = false };
    cfg_field_t *colour = config_bool(general, &kColourInfo, colour_initial);

    config_t *debug = config_group(group, &kDebugGroupInfo);

    cfg_bool_t verbose_initial = { .initial = false };
    cfg_field_t *verbose = config_bool(debug, &kVerboseLoggingInfo, verbose_initial);

    cfg_bool_t backtrace_initial = { .initial = false };
    cfg_field_t *backtrace = config_bool(debug, &kBacktraceInfo, backtrace_initial);

    default_options_t options = {
        .general_group = general,
        .print_help = help,
        .print_version = version,
        .enable_usage = argparse_usage,
        .enable_windows_style = windows_style,
        .colour_output = colour,

        .debug_group = debug,
        .log_verbose = verbose,
        .fancy_backtrace = backtrace,
    };

    return options;
}

static void fancy_panic_handler(panic_t panic, const char *msg, va_list args)
{
    bt_report_t report = bt_report_collect(get_global_arena());

    char *info = vformat(msg, args);

    char *detail = format("[%s:%zu] %s: %s", panic.file, panic.line, panic.function, info);

    file_config_t file_config = {
        .zeroth_line = false,
        .print_source = true,
        .print_header = true,
        .header_message = detail,
    };

    text_config_t config = {
        .config = file_config,
        .colours = &kColourDefault,
    };

    bt_report_finish(config, &report);

    abort();
}

int process_default_options(default_options_t options, tool_config_t config)
{
    const char *name = config.argv[0];

    bool log_verbose = cfg_bool_value(options.log_verbose);
    if (log_verbose)
    {
        ctu_log_update(true);
        ctu_log("enabled verbose logging");
    }

    bool fancy_backtrace = cfg_bool_value(options.fancy_backtrace);
    if (fancy_backtrace)
    {
        gPanicHandler = fancy_panic_handler;
        ctu_log("installed panic handler");
    }

    bool colour = cfg_bool_value(options.colour_output);
    format_context_t context = {
        .pallete = colour ? &kColourDefault : &kColourNone,
        .arena = config.arena,
    };

    bool show_help = cfg_bool_value(options.print_help);
    if (show_help)
    {
        format_config_t config_display = {
            .context = context,
            .io = config.io,
            .config = config.group,
            .print_usage = cfg_bool_value(options.enable_usage),
            .win_style = cfg_bool_value(options.enable_windows_style),
            .name = name
        };

        print_config(config_display);
        return EXIT_SHOULD_EXIT;
    }

    bool show_version = cfg_bool_value(options.print_version);
    if (show_version)
    {
        format_version_t version_display = {
            .context = context,
            .io = config.io,
            .version = config.version,
            .name = name
        };

        print_version(version_display);
        return EXIT_SHOULD_EXIT;
    }

    return EXIT_OK;
}

static void report_argparse_errors(io_t *io, format_context_t ctx, vector_t *args)
{
    size_t count = vector_len(args);
    if (count == 0) return;

    char *err = colour_text(ctx, eColourRed, "error");

    for (size_t i = 0; i < count; i++)
    {
        const char *arg = vector_get(args, i);
        io_printf(io, "%s: %s\n", err, arg);
    }
}

static void report_unknown_args(io_t *io, format_context_t ctx, vector_t *args)
{
    size_t count = vector_len(args);
    if (count == 0) return;

    char *unk = colour_text(ctx, eColourYellow, "unknown argument");

    for (size_t i = 0; i < count; i++)
    {
        const char *arg = vector_get(args, i);
        io_printf(io, "%s: %s\n", unk, arg);
    }
}

static int process_argparse_result(default_options_t options, tool_config_t config, ap_t *ap)
{
    int err = ap_parse_args(ap, config.argc, config.argv);

    bool colour = cfg_bool_value(options.colour_output);
    const colour_pallete_t *pallete = colour ? &kColourDefault : &kColourNone;

    vector_t *errors = ap_get_errors(ap);

    vector_t *unknown = ap_get_unknown(ap);
    size_t unknown_count = vector_len(unknown);

    format_context_t ctx = {
        .pallete = pallete,
        .arena = config.arena,
    };

    report_argparse_errors(config.io, ctx, errors);
    report_unknown_args(config.io, ctx, unknown);

    size_t count = ap_count_params(ap);
    vector_t *posargs = ap_get_posargs(ap);
    size_t posarg_count = vector_len(posargs);

    bool has_no_args = count == 0 && posarg_count == 0;

    if (!has_no_args && err == EXIT_OK)
    {
        return EXIT_OK;
    }

    if (has_no_args && unknown_count == 0)
    {
        io_printf(config.io, "no arguments provided\n");
    }

    format_config_t display = {
        .context = {
            .pallete = pallete,
            .arena = config.arena,
        },
        .io = config.io,
        .config = config.group,
        .print_usage = cfg_bool_value(options.enable_usage),
        .win_style = cfg_bool_value(options.enable_windows_style),
        .name = config.argv[0]
    };

    print_config(display);
    return EXIT_SHOULD_EXIT;
}

int parse_commands(default_options_t options, tool_config_t config)
{
    ap_t *ap = ap_new(config.group, config.arena);
    return parse_argparse(ap, options, config);
}

int parse_argparse(ap_t *ap, default_options_t options, tool_config_t config)
{
    int err = process_argparse_result(options, config, ap);
    if (err != EXIT_OK)
    {
        return err;
    }

    return process_default_options(options, config);
}

void default_init(void)
{
    bt_init();
    os_init();

    arena_t *arena = ctu_default_alloc();
    init_global_arena(arena);
    init_gmp_arena(arena);
}
