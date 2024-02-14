#include "ast.h"
/* autogenerated source, do not edit */

#include "arena/arena.h"
#include "scan/node.h"
#include "scan/scan.h"
#include "base/util.h"
#include "base/panic.h"
#include "std/vector.h"

static pl0x_ast_t *pl0x_ast_new(scan_t *scan, where_t where, pl0x_kind_t kind)
{
	arena_t *arena = scan_get_arena(scan);
	pl0x_ast_t *ast = ARENA_MALLOC(sizeof(pl0x_ast_t), NULL, "pl0x ast", arena);
	ast->kind = kind;
	ast->node = node_new(scan, where);
	return ast;
}
pl0x_ast_t *pl0x_ident(scan_t *scan, where_t where, const char *ident)
{
	pl0x_ident_t option;
	option.ident = ident;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xIdent);
	ast->ident = option;
	return ast;
}
pl0x_ast_t *pl0x_digit(scan_t *scan, where_t where, mpz_t digit)
{
	pl0x_digit_t option;
	mpz_init_set(option.digit, digit);
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xDigit);
	ast->digit = option;
	return ast;
}
pl0x_ast_t *pl0x_odd(scan_t *scan, where_t where, pl0x_ast_t *expr)
{
	pl0x_odd_t option;
	option.expr = expr;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xOdd);
	ast->odd = option;
	return ast;
}
pl0x_ast_t *pl0x_unary(scan_t *scan, where_t where, unary_t unary, pl0x_ast_t *operand)
{
	pl0x_unary_t option;
	option.unary = unary;
	option.operand = operand;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xUnary);
	ast->unary = option;
	return ast;
}
pl0x_ast_t *pl0x_binary(scan_t *scan, where_t where, binary_t binary)
{
	pl0x_binary_t option;
	option.binary = binary;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xBinary);
	ast->binary = option;
	return ast;
}
pl0x_ast_t *pl0x_compare(scan_t *scan, where_t where, compare_t compare)
{
	pl0x_compare_t option;
	option.compare = compare;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xCompare);
	ast->compare = option;
	return ast;
}
pl0x_ast_t *pl0x_assign(scan_t *scan, where_t where, const char *dst, pl0x_ast_t *src)
{
	pl0x_assign_t option;
	option.dst = dst;
	option.src = src;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xAssign);
	ast->assign = option;
	return ast;
}
pl0x_ast_t *pl0x_branch(scan_t *scan, where_t where, pl0x_ast_t *cond, pl0x_ast_t *then)
{
	pl0x_branch_t option;
	option.cond = cond;
	option.then = then;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xBranch);
	ast->branch = option;
	return ast;
}
pl0x_ast_t *pl0x_stmts(scan_t *scan, where_t where, vector_t *stmts)
{
	pl0x_stmts_t option;
	option.stmts = stmts;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xStmts);
	ast->stmts = option;
	return ast;
}
pl0x_ast_t *pl0x_procedure(scan_t *scan, where_t where, vector_t *locals, vector_t *body)
{
	pl0x_procedure_t option;
	option.locals = locals;
	option.body = body;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xProcedure);
	ast->procedure = option;
	return ast;
}
pl0x_ast_t *pl0x_value(scan_t *scan, where_t where, pl0x_ast_t *value)
{
	pl0x_value_t option;
	option.value = value;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xValue);
	ast->value = option;
	return ast;
}
pl0x_ast_t *pl0x_import(scan_t *scan, where_t where, vector_t *path)
{
	pl0x_import_t option;
	option.path = path;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xImport);
	ast->import = option;
	return ast;
}
pl0x_ast_t *pl0x_module(scan_t *scan, where_t where, vector_t *consts, vector_t *globals, vector_t *procedures, pl0x_ast_t *entry, vector_t *modspec, vector_t *imports)
{
	pl0x_module_t option;
	option.consts = consts;
	option.globals = globals;
	option.procedures = procedures;
	option.entry = entry;
	option.modspec = modspec;
	option.imports = imports;
	pl0x_ast_t *ast = pl0x_ast_new(scan, where, ePl0xModule);
	ast->module = option;
	return ast;
}
static size_t hash_combine(size_t hash, size_t value)
{
	hash ^= value + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}
static size_t hash_combine_vector(size_t hash, size_t (*fn)(const void *), const vector_t *vec)
{
	for (size_t i = 0; i < vector_len(vec); i++)
	{
		hash = hash_combine(hash, fn(vector_get(vec, i)));
	}
	return hash;
}
size_t pl0x_ast_hash(const pl0x_ast_t *ast)
{
	if (ast == NULL) return 0;
	size_t hash = 0;
	hash ^= ast->kind;

	switch (ast->kind)
	{
	case ePl0xIdent:
		hash = hash_combine(hash, str_hash(ast->ident.ident));
		break;
	case ePl0xDigit:
		hash = hash_combine(hash, mpz_get_ui(ast->digit.digit));
		break;
	case ePl0xOdd:
		hash = hash_combine(hash, pl0x_ast_hash(ast->odd.expr));
		break;
	case ePl0xUnary:
		hash = hash_combine(hash, ast->unary.unary);
		hash = hash_combine(hash, pl0x_ast_hash(ast->unary.operand));
		break;
	case ePl0xTwo:
		hash = hash_combine(hash, pl0x_ast_hash(ast->two.lhs));
		hash = hash_combine(hash, pl0x_ast_hash(ast->two.rhs));
		break;
	case ePl0xBinary:
		hash = hash_combine(hash, ast->binary.binary);
		break;
	case ePl0xCompare:
		hash = hash_combine(hash, ast->compare.compare);
		break;
	case ePl0xAssign:
		hash = hash_combine(hash, str_hash(ast->assign.dst));
		hash = hash_combine(hash, pl0x_ast_hash(ast->assign.src));
		break;
	case ePl0xBranch:
		hash = hash_combine(hash, pl0x_ast_hash(ast->branch.cond));
		hash = hash_combine(hash, pl0x_ast_hash(ast->branch.then));
		break;
	case ePl0xStmts:
		hash = hash_combine_vector(hash, (size_t(*)(const void*))pl0x_ast_hash, ast->stmts.stmts);
		break;
	case ePl0xDecl:
		hash = hash_combine(hash, str_hash(ast->decl.name));
		break;
	case ePl0xProcedure:
		hash = hash_combine_vector(hash, (size_t(*)(const void*))pl0x_ast_hash, ast->procedure.locals);
		hash = hash_combine_vector(hash, (size_t(*)(const void*))pl0x_ast_hash, ast->procedure.body);
		break;
	case ePl0xValue:
		hash = hash_combine(hash, pl0x_ast_hash(ast->value.value));
		break;
	case ePl0xImport:
		hash = hash_combine_vector(hash, (size_t(*)(const void*))str_hash, ast->import.path);
		break;
	case ePl0xModule:
		hash = hash_combine_vector(hash, (size_t(*)(const void*))pl0x_ast_hash, ast->module.consts);
		hash = hash_combine_vector(hash, (size_t(*)(const void*))pl0x_ast_hash, ast->module.globals);
		hash = hash_combine_vector(hash, (size_t(*)(const void*))pl0x_ast_hash, ast->module.procedures);
		hash = hash_combine(hash, pl0x_ast_hash(ast->module.entry));
		hash = hash_combine_vector(hash, (size_t(*)(const void*))str_hash, ast->module.modspec);
		hash = hash_combine_vector(hash, (size_t(*)(const void*))pl0x_ast_hash, ast->module.imports);
		break;
	default: NEVER("unknown ast type %d", ast->kind);
	}

	return hash;
}
