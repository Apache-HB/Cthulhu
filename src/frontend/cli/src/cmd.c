// SPDX-License-Identifier: GPL-3.0-only

#include "cmd.h"

#include "config/config.h"

#include "format/notify.h"
#include "std/vector.h"
#include "std/str.h"

#include "notify/notify.h"

#include "argparse/argparse.h"

static const cfg_info_t kConfigInfo = {
    .name = "cli",
    .brief = "Cthulhu CLI configuration options",
};

static const cfg_info_t kReportInfo = {
    .name = "reports",
    .brief = "Reporting options"
};

static const char *const kLangShortArgs[] = { "l", NULL };
static const char *const kLangLongArgs[] = { "lang", NULL };

static const cfg_info_t kLang = {
    .name = "lang",
    .brief = "Load a language driver",
    .short_args = kLangShortArgs,
    .long_args = kLangLongArgs,
};

static const char *const kIrShortArgs[] = { "ir", NULL };
static const char *const kIrLongArgs[] = { "emit-ir", NULL };

static const cfg_info_t kEmitIr = {
    .name = "emit-ssa",
    .brief = "Emit SSA IR to the output directory",
    .short_args = kIrShortArgs,
    .long_args = kIrLongArgs,
};

static const char *const kWarnAsErrorShortArgs[] = { "Werror", NULL };

static const cfg_info_t kWarnAsError = {
    .name = "warn-as-error",
    .brief = "Treat warnings as errors",
    .short_args = kWarnAsErrorShortArgs
};

static const char *const kReportLimitShortArgs[] = { "fmax-errors", NULL };

static const cfg_info_t kReportLimit = {
    .name = "max-errors",
    .brief = "Limit the number of reports",
    .short_args = kReportLimitShortArgs
};

static const char *const kOutputDirShortArgs[] = { "o", NULL };
static const char *const kOutputDirLongArgs[] = { "dir", NULL };

static const cfg_info_t kOutputDir = {
    .name = "output-dir",
    .brief = "Output directory for generated files",
    .short_args = kOutputDirShortArgs,
    .long_args = kOutputDirLongArgs,
};

static const char *const kReportStyleShortArgs[] = { "r", NULL };
static const char *const kReportStyleLongArgs[] = { "report", NULL };

static const cfg_info_t kReportStyle = {
    .name = "report-style",
    .brief = "Report style to use",
    .short_args = kReportStyleShortArgs,
    .long_args = kReportStyleLongArgs,
};

static const cfg_choice_t kReportStyleChoices[] = {
    { "simple", eTextSimple },
    { "complex", eTextComplex },
};

tool_t make_tool(arena_t *arena)
{
    cfg_group_t *config = config_root(&kConfigInfo, arena);

    default_options_t options = get_default_options(config);

    cfg_field_t *lang_field = config_vector(config, &kLang, NULL);

    cfg_field_t *emit_ir_field = config_bool(config, &kEmitIr, false);

    cfg_field_t *output_dir_field = config_string(config, &kOutputDir, NULL);

    cfg_group_t *report_group = config_group(config, &kReportInfo);
    cfg_field_t *warn_as_error_field = config_bool(report_group, &kWarnAsError, false);

    cfg_int_t report_limit_options = {.initial = 20, .min = 0, .max = 1000};
    cfg_field_t *report_limit_field = config_int(report_group, &kReportLimit, report_limit_options);

    cfg_enum_t report_style_options = {
        .options = kReportStyleChoices,
        .count = (sizeof(kReportStyleChoices) / sizeof(cfg_choice_t)),
        .initial = eTextSimple,
    };
    cfg_field_t *report_style_field = config_enum(report_group, &kReportStyle, report_style_options);

    tool_t tool = {
        .config = config,
        .options = options,

        .langs = lang_field,

        .emit_ssa = emit_ir_field,
        .output_dir = output_dir_field,

        .warn_as_error = warn_as_error_field,
        .report_limit = report_limit_field,
        .report_style = report_style_field,
    };

    return tool;
}
