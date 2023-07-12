#include "common/common.h"

#include "cthulhu/hlir/query.h"

#include "std/str.h"
#include "std/map.h"
#include "std/set.h"
#include "std/vector.h"

#include "std/typed/vector.h"

#include "base/memory.h"
#include "base/panic.h"

typedef struct ssa_compile_t {
    /// result data

    vector_t *modules; ///< vector<ssa_module>

    map_t *deps;

    /// internal data

    map_t *globals; ///< map<h2, ssa_symbol>
    map_t *locals; ///< map<h2, ssa_symbol>

    ssa_block_t *currentBlock;
    ssa_symbol_t *currentSymbol;

    vector_t *path;
} ssa_compile_t;

static void add_dep(ssa_compile_t *ssa, const ssa_symbol_t *symbol, const ssa_symbol_t *dep)
{
    set_t *set = map_get_ptr(ssa->deps, symbol);
    if (set == NULL)
    {
        set = set_new(8);
        map_set_ptr(ssa->deps, symbol, set);
    }

    set_add_ptr(set, dep);
}

static ssa_symbol_t *symbol_create(ssa_compile_t *ssa, const h2_t *tree)
{
    const char *name = h2_get_name(tree);
    ssa_type_t *type = ssa_type_from(h2_get_type(tree));
    const h2_attrib_t *attrib = h2_get_attrib(tree);

    ssa_symbol_t *symbol = ctu_malloc(sizeof(ssa_symbol_t));
    symbol->linkage = attrib->link;
    symbol->visibility = attrib->visibility;
    symbol->linkName = attrib->mangle;

    symbol->locals = NULL;

    symbol->name = name;
    symbol->type = type;
    symbol->value = NULL;
    symbol->entry = NULL;

    symbol->blocks = vector_new(4);

    return symbol;
}

static ssa_module_t *module_create(ssa_compile_t *ssa, const char *name)
{
    vector_t *path = vector_clone(ssa->path);

    ssa_module_t *mod = ctu_malloc(sizeof(ssa_module_t));
    mod->name = name;
    mod->path = path;

    mod->globals = vector_new(32);
    mod->functions = vector_new(32);

    return mod;
}

static ssa_operand_t add_step(ssa_compile_t *ssa, ssa_step_t step)
{
    const ssa_block_t *bb = ssa->currentBlock;
    size_t index = typevec_len(bb->steps);
    typevec_push(bb->steps, &step);

    ssa_operand_t operand = {
        .kind = eOperandReg,
        .vregContext = bb,
        .vregIndex = index
    };

    return operand;
}

static ssa_operand_t compile_tree(ssa_compile_t *ssa, const h2_t *tree)
{
    switch (tree->kind)
    {
    case eHlir2ExprEmpty: {
        ssa_operand_t operand = {
            .kind = eOperandEmpty
        };
        return operand;
    }
    case eHlir2ExprDigit:
    case eHlir2ExprBool:
    case eHlir2ExprUnit:
    case eHlir2ExprString: {
        ssa_operand_t operand = {
            .kind = eOperandImm,
            .value = ssa_value_from(tree)
        };
        return operand;
    }
    case eHlir2ExprUnary: {
        ssa_operand_t expr = compile_tree(ssa, tree->operand);
        ssa_step_t step = {
            .opcode = eOpUnary,
            .unary = {
                .operand = expr,
                .unary = tree->unary
            }
        };
        return add_step(ssa, step);
    }
    case eHlir2ExprBinary: {
        ssa_operand_t lhs = compile_tree(ssa, tree->lhs);
        ssa_operand_t rhs = compile_tree(ssa, tree->rhs);
        ssa_step_t step = {
            .opcode = eOpBinary,
            .binary = {
                .lhs = lhs,
                .rhs = rhs,
                .binary = tree->binary
            }
        };
        return add_step(ssa, step);
    }

    default: NEVER("unhandled tree kind %d", h2_to_string(tree));
    }
}

static void add_module_globals(ssa_compile_t *ssa, ssa_module_t *mod, map_t *globals)
{
    map_iter_t iter = map_iter(globals);
    while (map_has_next(&iter))
    {
        map_entry_t entry = map_next(&iter);

        const h2_t *tree = entry.value;
        ssa_symbol_t *global = symbol_create(ssa, tree);

        vector_push(&mod->globals, global);
        map_set_ptr(ssa->globals, tree, global);
    }
}

static void compile_module(ssa_compile_t *ssa, const h2_t *tree)
{
    const char *id = h2_get_name(tree);
    ssa_module_t *mod = module_create(ssa, id);

    add_module_globals(ssa, mod, h2_module_tag(tree, eSema2Values));

    vector_push(&ssa->modules, mod);
    vector_push(&ssa->path, (char*)id);

    map_t *children = h2_module_tag(tree, eSema2Modules);
    map_iter_t iter = map_iter(children);
    while (map_has_next(&iter))
    {
        map_entry_t entry = map_next(&iter);

        compile_module(ssa, entry.value);
    }

    vector_drop(ssa->path);
}

static void begin_compile(ssa_compile_t *ssa, ssa_symbol_t *symbol)
{
    ssa_block_t *bb = ctu_malloc(sizeof(ssa_block_t));
    bb->name = "entry";
    bb->steps = typevec_new(sizeof(ssa_step_t), 4);

    vector_push(&symbol->blocks, bb);
    symbol->entry = bb;
    ssa->currentBlock = bb;
    ssa->currentSymbol = symbol;
}

ssa_result_t ssa_compile(map_t *mods)
{
    ssa_compile_t ssa = {
        .modules = vector_new(4),
        .deps = map_optimal(64),

        .globals = map_optimal(32),
        .locals = map_optimal(32)
    };

    map_iter_t iter = map_iter(mods);
    while (map_has_next(&iter))
    {
        map_entry_t entry = map_next(&iter);

        ssa.path = str_split(entry.key, ".");
        compile_module(&ssa, entry.value);
    }

    map_iter_t globals = map_iter(ssa.globals);
    while (map_has_next(&globals))
    {
        map_entry_t entry = map_next(&globals);

        const h2_t *tree = entry.key;
        ssa_symbol_t *global = entry.value;

        begin_compile(&ssa, global);

        ssa_operand_t value = compile_tree(&ssa, tree->global);
        ssa_step_t ret = {
            .opcode = eOpReturn,
            .ret = {
                .value = value
            }
        };
        add_step(&ssa, ret);
    }

    ssa_result_t result = {
        .modules = ssa.modules,
        .deps = ssa.deps
    };

    return result;
}
