%define parse.error verbose
%define api.pure full
%lex-param { void *scan }
%parse-param { void *scan } { scan_t x }
%locations
%expect 0
%define api.prefix {ctu}

%code top {
    #include "interop/flex.h"
}

%code requires {
    #include "ast.h"
    #include "scan.h"
    #define YYSTYPE CTUSTYPE
    #define YYLTYPE CTULTYPE
}

%{
int ctulex();
void ctuerror(where_t *where, void *state, scan_t scan, const char *msg);
%}

%union {
    bool boolean;
    char *ident;
    struct {
        char *data;
        size_t length;
    } string;

    mpz_t mpz;

    ast_t *ast;
    vector_t *vector;
    funcparams_t funcparams;
}

%token<ident>
    IDENT "identifier"

%token<string>
    STRING "string literal"

%token<mpz>
    INTEGER "integer literal"

%token<boolean>
    BOOLEAN "boolean literal"

%token
    AT "`@`"
    COLON "`:`"
    COLON2 "`::`"
    SEMICOLON "`;`"
    COMMA "`,`"
    DOT "`.`"
    DOT3 "`...`"
    ARROW "`->`"
    LSHIFT "`<<`"
    RSHIFT "`>>`"

    BEGIN_TEMPLATE "`!<` (template)"
    END_TEMPLATE "`>` (template)"

    NOT "`!`"
    EQUALS "`=`"
    EQ "`==`"
    NEQ "`!=`"

    ADD "`+`"
    SUB "`-`"
    MUL "`*`"
    DIV "`/`"
    MOD "`%`"

    AND "`&&`"
    OR "`||`"

    BITAND "`&`"
    BITOR "`|`"
    BITXOR "`^`"

    LPAREN "`(`"
    RPAREN "`)`"
    LSQUARE "`[`"
    RSQUARE "`]`"
    LBRACE "`{`"
    RBRACE "`}`"

    GT "`>`"
    GTE "`>=`"
    LT "`<`"
    LTE "`<=`"

    MODULE "`module`"
    IMPORT "`import`"
    EXPORT "`export`"

    DEF "`def`"
    VAR "`var`"
    FINAL "`final`"
    CONST "`const`"

    OBJECT "`object`"
    STRUCT "`struct`"
    UNION "`union`"
    VARIANT "`variant`"
    TYPE "`type`"

    IF "`if`"
    ELSE "`else`"
    WHILE "`while`"
    FOR "`for`"
    BREAK "`break`"
    CONTINUE "`continue`"
    RETURN "`return`"

    LAMBDA "`lambda`"
    AS "`as`"

    NIL "`null`"

    MATCH "`match`"
    DEFAULT "`default`"
    CASE "`case`"
    SELECT "`select`"
    ORDER "`order`"
    ON "`on`"
    FROM "`from`"
    WHERE "`where`"
    WHEN "`when`"
    THEN "`then`"

    IN "`in`"
    OUT "`out`"
    INOUT "`inout`"

    PRIVATE "`private`"
    PUBLIC "`public`"
    PROTECTED "`protected`"

    ASYNC "`async`"
    AWAIT "`await`"
    YIELD "`yield`"

    CONTRACT "`contract`"
    REQUIRES "`requires`"
    ASSERT "`assert`"
    ENSURES "`ensures`"
    INVARIANT "`invariant`"

    SIZEOF "`sizeof`"
    ALIGNOF "`alignof`"
    OFFSETOF "`offsetof`"
    TYPEOF "`typeof`"
    UUIDOF "`uuidof`"

    TRY "`try`"
    CATCH "`catch`"
    FINALLY "`finally`"
    THROW "`throw`"

    COMPILED "`compile`"
    STATIC "`static`"

%type<ast>
    modspec decl innerdecl
    attrib
    structdecl uniondecl variantdecl aliasdecl
    field variant 
    funcdecl funcresult funcbody funcsig
    vardecl
    param
    stmt stmts
    type import
    expr postfix unary multiply add 
    compare equality shift bits xor and or
    primary else

%type<vector>
    path decls decllist
    fieldlist aggregates
    typelist imports importlist
    variants variantlist
    stmtlist paramlist
    args

%type<funcparams>
    funcparams opttypes types

%type<boolean>
    mut

%start program

%%

program: modspec imports decls { scan_set(x, ast_program(x, @$, $1, $2, $3)); }
    ;

imports: %empty { $$ = vector_new(0); }
    | importlist { $$ = $1; }
    ;

importlist: import { $$ = vector_init($1); }
    | importlist import { vector_push(&$1, $2); $$ = $1; }
    ;

import: IMPORT path SEMICOLON { $$ = ast_import(x, @$, $2); }
    ;

decls: %empty { $$ = vector_new(0); }
    | decllist { $$ = $1; }
    ;

decllist: decl { $$ = vector_init($1); }
    | decllist decl { vector_push(&$1, $2); $$ = $1; }
    ;

decl: innerdecl { $$ = $1; }
    | attrib innerdecl { $2->attrib = $1; $$ = $2; }
    ;

attrib: AT IDENT LPAREN args RPAREN { $$ = ast_attribute(x, @$, $2, $4); }
    ;

innerdecl: structdecl { $$ = $1; }
    | uniondecl { $$ = $1; }
    | aliasdecl { $$ = $1; }
    | variantdecl { $$ = $1; }
    | funcdecl { $$ = $1; }
    | vardecl { $$ = $1; }
    ;

funcdecl: DEF IDENT funcsig funcbody { $$ = ast_function(x, @$, $2, $3, $4); }
    ;

vardecl: mut IDENT EQUALS expr SEMICOLON { $$ = ast_variable(x, @$, $2, $1, $4); }
    ;

mut: VAR { $$ = true; }
    | CONST { $$ = false; }
    ;

funcsig: funcparams funcresult { $$ = ast_closure(x, @$, $1.params, $1.variadic, $2); }
    ;

funcresult: %empty { $$ = NULL; }
    | COLON type { $$ = $2; }
    ;

funcparams: %empty { $$ = funcparams_new(vector_of(0), false); }
    | LPAREN paramlist RPAREN { $$ = funcparams_new($2, false); }
    | LPAREN paramlist COMMA DOT3 RPAREN { $$ = funcparams_new($2, true); }
    ;

paramlist: param { $$ = vector_init($1); }
    | paramlist COMMA param { vector_push(&$1, $3); $$ = $1; }
    ;

param: IDENT COLON type { $$ = ast_param(x, @$, $1, $3); }
    ;

funcbody: SEMICOLON { $$ = NULL; }
    | EQUALS expr SEMICOLON { $$ = ast_stmts(x, @$, vector_init(ast_return(x, @2, $2))); }
    | stmts { $$ = $1; }
    ;

aliasdecl: TYPE IDENT EQUALS type SEMICOLON { $$ = ast_typealias(x, @$, $2, $4); }
    ;

structdecl: STRUCT IDENT aggregates { $$ = ast_structdecl(x, @$, $2, $3); }
    ;

uniondecl: UNION IDENT aggregates { $$ = ast_uniondecl(x, @$, $2, $3); }
    ;

variantdecl: VARIANT IDENT LBRACE variants RBRACE { $$ = ast_variantdecl(x, @$, $2, $4); }
    ;

variants: %empty { $$ = vector_new(0); }
    | variantlist { $$ = $1; }
    ;

variantlist: variant { $$ = vector_init($1); }
    | variantlist COMMA variant { vector_push(&$1, $3); $$ = $1; }
    ;

variant: IDENT { $$ = ast_field(x, @$, $1, NULL); }
    | IDENT LPAREN type RPAREN { $$ = ast_field(x, @$, $1, $3); }
    ;

aggregates: LBRACE fieldlist RBRACE { $$ = $2; }
    ;

fieldlist: field { $$ = vector_init($1); }
    | fieldlist field { vector_push(&$1, $2); $$ = $1; }
    ;

field: IDENT COLON type SEMICOLON { $$ = ast_field(x, @$, $1, $3); }
    ;

modspec: %empty { $$ = NULL; }
    | MODULE path SEMICOLON { $$ = ast_module(x, @$, $2); }
    ;

stmtlist: stmt { $$ = vector_init($1); }
    | stmtlist stmt { vector_push(&$1, $2); $$ = $1; }
    ;

stmts: LBRACE RBRACE { $$ = ast_stmts(x, @$, vector_of(0)); }
    | LBRACE stmtlist RBRACE { $$ = ast_stmts(x, @$, $2); }
    ;

stmt: stmts { $$ = $1; }
    | RETURN expr SEMICOLON { $$ = ast_return(x, @$, $2); }
    | WHILE expr stmts else { $$ = ast_while(x, @$, $2, $3, $4); }
    | BREAK SEMICOLON { $$ = ast_break(x, @$); }
    | CONTINUE SEMICOLON { $$ = ast_continue(x, @$); }
    | vardecl { $$ = $1; }
    | expr SEMICOLON { $$ = $1; }
    ;

else: %empty { $$ = NULL; }
    | ELSE stmts { $$ = $2; }
    ;

type: path { $$ = ast_typename(x, @$, $1); }
    | MUL type { $$ = ast_pointer(x, @$, $2, false); }
    | LSQUARE MUL RSQUARE type { $$ = ast_pointer(x, @$, $4, true); }
    | LSQUARE expr RSQUARE type { $$ = ast_array(x, @$, $2, $4); }
    | DEF LPAREN opttypes RPAREN ARROW type { $$ = ast_closure(x, @$, $3.params, $3.variadic, $6); } 
    | LPAREN type RPAREN { $$ = $2; }
    ;

opttypes: %empty { $$ = funcparams_new(vector_of(0), false); }
    | types { $$ = $1; }
    ;

types: typelist { $$ = funcparams_new($1, false); }
    | typelist COMMA DOT3 { $$ = funcparams_new($1, true); }
    ;

typelist: type { $$ = vector_init($1); }
    | typelist COMMA type { vector_push(&$1, $3); $$ = $1; }
    ;

primary: LPAREN expr RPAREN { $$ = $2; }
    | INTEGER { $$ = ast_digit(x, @$, $1); }
    | path { $$ = ast_name(x, @$, $1); }
    | BOOLEAN { $$ = ast_bool(x, @$, $1); }
    | STRING { $$ = ast_string(x, @$, $1.data, $1.length); }
    ;

postfix: primary { $$ = $1; }
    | postfix LPAREN args RPAREN { $$ = ast_call(x, @$, $1, $3); }
    ;

unary: postfix { $$ = $1; }
    ;

multiply: unary { $$ = $1; }
    | multiply MUL unary { $$ = ast_binary(x, @$, eBinaryMul, $1, $3); }
    | multiply DIV unary { $$ = ast_binary(x, @$, eBinaryDiv, $1, $3); }
    | multiply MOD unary { $$ = ast_binary(x, @$, eBinaryRem, $1, $3); }
    ;

add: multiply { $$ = $1; }
    | add ADD multiply { $$ = ast_binary(x, @$, eBinaryAdd, $1, $3); }
    | add SUB multiply { $$ = ast_binary(x, @$, eBinarySub, $1, $3); }
    ;

compare: add { $$ = $1; }
    | compare LT add { $$ = ast_compare(x, @$, eCompareLt, $1, $3); }
    | compare GT add { $$ = ast_compare(x, @$, eCompareGt, $1, $3); }
    | compare LTE add { $$ = ast_compare(x, @$, eCompareLte, $1, $3); }
    | compare GTE add { $$ = ast_compare(x, @$, eCompareGte, $1, $3); }
    ;

equality: compare { $$ = $1; }
    ;

shift: equality { $$ = $1; }
    ;

bits: shift { $$ = $1; }
    ;

xor: bits { $$ = $1; }
    ;

and: xor { $$ = $1; }
    ;

or: and { $$ = $1; }
    ;

expr: or { $$ = $1; }
    ;

args: expr { $$ = vector_init($1); }
    | args COMMA expr { vector_push(&$1, $3); $$ = $1; }
    ; 

path: IDENT { $$ = vector_init($1); }
    | path COLON2 IDENT { vector_push(&$1, $3); $$ = $1; }
    ;

%%
