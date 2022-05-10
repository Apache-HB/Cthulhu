#include "cthulhu/ast/compile.h"
#include "cthulhu/interface/interface.h"
#include "cthulhu/interface/runtime.h"

#include "sema.h"

#include "ctu-bison.h"
#include "ctu-flex.h"

CT_CALLBACKS(kCallbacks, ctu);

void ctu_parse_file(runtime_t *runtime, compile_t *compile)
{
    UNUSED(runtime);
    compile->ast = compile_file(compile->scanner, &kCallbacks);
}

static void ctu_init_compiler(runtime_t *runtime)
{
    UNUSED(runtime);
}

const driver_t kDriver = {
    .name = "Cthulhu",
    .version = NEW_VERSION(1, 0, 0),

    .fnInitCompiler = ctu_init_compiler,
    .fnParseFile = ctu_parse_file,
    .fnForwardDecls = ctu_forward_decls,
    .fnResolveImports = ctu_process_imports,
    .fnCompileModule = ctu_compile_module,
};

driver_t get_driver()
{
    return kDriver;
}
