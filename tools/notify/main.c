#include "io/io.h"

#include "memory/memory.h"

#include "notify/notify.h"
#include "notify/text.h"

#include "report/report.h"

#include "scan/node.h"
#include "stacktrace/stacktrace.h"
#include "std/vector.h"
#include <stdio.h>

const char *kSampleSourceLhs =
    "module multi.lhs;\n"   // 1
    "\n"                    // 2
    "import multi.rhs,\n"   // 3
    "       main;\n"        // 4
    "\n"                    // 5
    "procedure lhs;\n"      // 3
    "begin\n"               // 4
    "    x := x + 1;\n"     // 5
    "    if x < LIMIT then\n" // 6
    "        call rhs\n"    // 7
    "end;\n"                // 8
    ".\n";                  // 9

const char *kSampleSourceRhs =
    "module multi.rhs;\n"   // 1
    "\n"                    // 2
    "import multi.lhs,\n"   // 3
    "       main;\n"        // 4
    "\n"                    // 5
    "procedure rhs;\n"      // 6
    "begin\n"               // 7
    "    x := x + 1;\n"     // 8
    "    if x < LIMIT then\n" // 9
    "        call lhs\n"    // 10
    "end;\n"                // 11
    ".\n";                  // 12

const char *kSampleSourceMain =
    "module main;\n"        // 1
    "\n"                    // 2
    "import multi.lhs,\n"   // 3
    "       multi.rhs;\n"   // 4
    "\n"                    // 5
    "const LIMIT = 25;\n"   // 6
    "var x;\n"              // 7
    "\n"                    // 8
    "procedure entry;\n"    // 9
    "begin\n"               // 10
    "    x := 0;\n"         // 11
    "    call lhs;\n"       // 12
    "    ! x\n"             // 13
    "end;\n"                // 14
    "\n"                    // 15
    "call entry\n"          // 16
    ".\n";                  // 17

static const diagnostic_t kInfoDiagnostic = {
    .severity = eSeverityInfo,
    .id = "T0001",
    .brief = "Test diagnostic at info level",
    .description =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit.\n"
        "Sed non risus. Suspendisse lectus tortor, dignissim sit amet,\n"
        "adipiscing nec, ultricies sed, dolor. Cras elementum ultrices diam.\n"
        "Maecenas ligula massa, varius a, semper congue, euismod non, mi.\n"
        "Proin porttitor, orci nec nonummy molestie, enim est eleifend mi,\n"
        "non fermentum diam nisl sit amet erat. Duis semper. Duis arcu massa,\n"
        "scelerisque vitae, consequat in, pretium a, enim. Pellentesque congue.\n"
};

static const diagnostic_t kUndefinedFunctionName = {
    .severity = eSeverityFatal,
    .id = "G0001",
    .brief = "Undefined function name",
    .description =
        "A function name must be visible in the current scope\n"
        "to be used in a call expression or statement.\n"
};

static const diagnostic_t kUnresolvedImport = {
    .severity = eSeverityFatal,
    .id = "G0002",
    .brief = "Unresolved import",
    .description =
        "An import statement must refer to a valid module.\n"
        "The module must be visible in the current scope.\n"
};

static const diagnostic_t kReservedName = {
    .severity = eSeverityFatal,
    .id = "G0003",
    .brief = "Reserved name",
    .description =
        "A reserved name cannot be used as an identifier.\n"
        "Reserved names are keywords and builtin names.\n"
};

void event_simple(logger_t *logs)
{
    event_t *event = msg_notify(logs, &kInfoDiagnostic, node_builtin(), "test");
    msg_append(event, node_builtin(), "hello %s", "world");
}

void event_missing_call(logger_t *logs, scan_t *scan_main, scan_t *scan_lhs)
{
    where_t where = {
        .first_line = 11,
        .last_line = 11,
        .first_column = 4,
        .last_column = 4 + 8
    };

    node_t *node = node_new(scan_main, where);

    where_t where2 = {
        .first_line = 8,
        .last_line = 8,
        .first_column = 4,
        .last_column = 4 + 8
    };

    node_t *node2 = node_new(scan_lhs, where2);

    where_t where3 = {
        .first_line = 12,
        .last_line = 12,
        .first_column = 8,
        .last_column = 8 + 3
    };

    node_t *node3 = node_new(scan_lhs, where3);

    event_t *event = msg_notify(logs, &kUndefinedFunctionName, node, "undefined function name `%s`", "lhs");
    msg_note(event, "did you mean `%s`?", "rhs");
    msg_append(event, node, "function called here");
    msg_append(event, node, "function called here but with a different message");
    msg_append(event, node2, "function defined here");
    msg_append(event, node3, "foo bar");

    msg_append(event, node_builtin(), "builtin node");
}

void event_invalid_import(logger_t *logs, scan_t *scan, scan_t *scan_rhs)
{
    where_t where = {
        .first_line = 2,
        .last_line = 2,
        .first_column = 7,
        .last_column = 7 + 9
    };

    node_t *node = node_new(scan, where);

    where_t where2 = {
        .first_line = 3,
        .last_line = 3,
        .first_column = 7,
        .last_column = 7 + 9
    };

    node_t *node2 = node_new(scan_rhs, where2);

    event_t *event = msg_notify(logs, &kUnresolvedImport, node, "unresolved import `%s`", "multi.lhs");
    msg_note(event, "did you mean `%s`?", "multi.rhs");
    msg_note(event, "did you mean `%s`?", "multi.rhx");
    msg_append(event, node, "import statement here");
    msg_append(event, node2, "module declaration here");
}

void event_invalid_function(logger_t *logs, scan_t *scan)
{
    where_t where = {
        .first_line = 8,
        .last_line = 13,
        .first_column = 0,
        .last_column = 4
    };

    node_t *node = node_new(scan, where);

    event_t *event = msg_notify(logs, &kReservedName, node, "reserved name `%s`", "entry");
    msg_append(event, node, "procedure declaration here");
    msg_note(event, "did you mean `%s`?", "main");
}

static scan_t *scan_string(const char *name, const char *lang, const char *source)
{
    io_t *io = io_string(name, source);
    return scan_io(begin_reports(), lang, io, ctu_default_alloc());
}

void print_backtrace(void)
{
    io_t *io = io_blob("backtrace", 0x1000, eAccessWrite | eAccessText);
    text_config_t config = {
        .config = {
            .zeroth_line = false
        },
        .colours = kDefaultColour,
        .io = io
    };

    bt_report_t *report = bt_report_collect();

    bt_report_finish(config, report);

    const void *data = io_map(io);
    size_t size = io_size(io);

    fwrite(data, size, 1, stdout);
}

int recurse(int x)
{
    if (x == 0)
    {
        print_backtrace();
        return 0;
    }

    return recurse(x - 1);
}

int main()
{
    stacktrace_init();
    os_init();
    scan_init();

    init_global_alloc(ctu_default_alloc());
    init_gmp_alloc(ctu_default_alloc());

    logger_t *logs = log_new();
    msg_diagnostic(logs, &kInfoDiagnostic);
    msg_diagnostic(logs, &kUndefinedFunctionName);
    msg_diagnostic(logs, &kUnresolvedImport);
    msg_diagnostic(logs, &kReservedName);

    scan_t *scan_main = scan_string("sample.pl0", "PL/0", kSampleSourceMain);
    scan_t *scan_lhs = scan_string("lhs.mod", "Oberon-2", kSampleSourceLhs);
    scan_t *scan_rhs = scan_string("rhs.ctu", "Cthulhu", kSampleSourceRhs);

    event_simple(logs);
    event_missing_call(logs, scan_main, scan_lhs);
    event_invalid_import(logs, scan_main, scan_rhs);
    event_invalid_function(logs, scan_main);

    io_t *io_rich = io_blob("rich_test", 0x1000, eAccessWrite);
    io_t *io_simple = io_blob("simple_test", 0x1000, eAccessWrite);

    text_config_t config = {
        .config = {
            .zeroth_line = false,
        },
        .colours = kDefaultColour,
        .io = io_rich
    };

    text_config_t config2 = {
        .config = {
            .zeroth_line = false,
        },
        .colours = kDefaultColour,
        .io = io_simple
    };

    vector_t *events = log_events(logs);
    size_t count = vector_len(events);

    for (size_t i = 0; i < count; i++)
    {
        event_t *event = vector_get(events, i);
        text_report_rich(config, event);
        text_report_simple(config2, event);

        if (i != count - 1)
        {
            io_printf(io_rich, "\n");
            io_printf(io_simple, "\n");
        }
    }

    const void *data1 = io_map(io_rich);
    size_t size1 = io_size(io_rich);

    fprintf(stdout, "=== rich text ===\n\n");

    fwrite(data1, size1, 1, stdout);

    fprintf(stdout, "\n=== simple text ===\n\n");

    const void *data2 = io_map(io_simple);
    size_t size2 = io_size(io_simple);

    fwrite(data2, size2, 1, stdout);

    fprintf(stdout, "\n=== backtrace ===\n\n");

    recurse(15);
    recurse(1000);
}
