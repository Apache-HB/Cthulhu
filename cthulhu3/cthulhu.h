#pragma once

#include <unordered_set>
#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>
#include <span>

namespace cthulhu {

using text = const std::string*;

// string interning pool

struct pool {
    text intern(const std::string& string);
private:
    std::unordered_set<std::string> data;
};

// input stream

struct stream {
    virtual ~stream() { }
    virtual char next() = 0;
};

struct file_stream : stream {
    file_stream(const char* path);
    file_stream(FILE* source);

    virtual ~file_stream() override;
    virtual char next() override;
private:
    FILE* file;
};

struct text_stream : stream {
    text_stream(std::string_view string);

    virtual ~text_stream() override;
    virtual char next() override;
private:
    std::string_view string;
    size_t offset;
};

// a range in source

struct range {
    size_t offset;
    size_t length;
};

// a pretty position in source

struct location {
    size_t line;
    size_t column;
};

// an integer literal token

struct number {
    size_t digit;
    text suffix;
};

// a keyword

enum struct key : int {
    INVALID = 0,

    // keywords
    RECORD,
    USING,

    // reserved keywords

    // operators
    LPAREN,
    RPAREN,
    LSQUARE,
    RSQUARE,
    LBRACE,
    RBRACE,
    SEMI,
    COMMA,
    DOT,
    DOT2,
    DOT3,
    ASSIGN,
    AT,
    COLON,
    COLON2,
    QUESTION,

    ADD,
    ADDEQ,
    SUB,
    SUBEQ,
    DIV,
    DIVEQ,
    MUL,
    MULEQ,
    MOD,
    MODEQ,

    // templates
    BEGIN,
    END,

    NOT,
    BITNOT,
    
    SHL,
    SHLEQ,

    SHR,
    SHREQ,

    BITXOR, 
    BITXOREQ,
    BITAND,
    BITANDEQ,
    BITOR,
    BITOREQ,

    EQ,
    NEQ,
    AND,
    OR,
    GT,
    GTE,
    LT,
    LTE
};

// a token from a token stream

struct lexer;

struct token {
    enum kind {
        IDENT, // identifier
        KEY, // keyword
        STRING, // string literal
        CHAR, // char literal
        INT, // integer literal
        END, // end of file

        MONOSTATE, // this token doesnt exist

        STRING_EOF, // string wasnt terminated
        STRING_LINE, // newline found in string
        INVALID_ESCAPE, // invalid escaped character in string
        LEADING_ZERO, // an integer literal started with a 0
        INT_OVERFLOW, // integer literal was too large
        UNRECOGNIZED_CHAR // unrecognized character in stream
    };

    union content {
        text id; // type::IDENT
        text str; // type::STRING
        key word; // type::KEY
        text letters; // type::CHAR
        number digit; // type::INT
    };

    token(kind type, content data = {}) : token({}, type, data) { }

    token(range range = {}, kind type = MONOSTATE, content data = {})
        : range(range)
        , type(type)
        , data(data)
    { }

    // return true if the token is not an error, monostate, or EOF
    bool valid() const;

    // return true if the token is an error token
    bool error() const;

    // return a string repr of the type of this token
    const char* repr() const;

    range range;
    kind type;
    content data;
};

// a token stream

struct lexer {
    using key_map = std::unordered_map<text, key>;

    lexer(stream* source, std::shared_ptr<pool> idents = std::make_shared<pool>());

    token read();



    token ident(char c);
    token rstring();
    token string();
    token letters();
    token digit(char c);
    token symbol(char c);

    text consume(char delim, token::kind* kind);

    token make(token::kind kind, token::content data = {}) const;

    char next();
    char peek();
    char skip();
    bool eat(char c);
    text intern(const std::string& string);
    std::string collect(char c, bool(*filter)(char));

    std::string slice(const range& range) const;

private:
    stream* source;
    char ahead;
    std::shared_ptr<pool> idents;
    
    key_map keys;

    std::string text;

    size_t depth = 0;
    size_t start = 0;
    size_t offset = 0;
};

namespace ast {

// we break style here because stuff like return is a keyword in C++

struct Node { };

struct Stmt : Node { };
struct Decl : Stmt { };
struct Expr : Stmt { };
struct Type : Node { };
struct Ident : Node { };

struct Name : Node {
    Ident* name;
    Type* type;
};

struct Value : Stmt {
    std::vector<Name*> names;
    Expr* expr;
};

struct Compound : Stmt {
    std::vector<Stmt*> body;
};

struct Return : Stmt {
    Expr* expr;
};

struct While : Stmt {
    Ident* label;
    Expr* cond;
    Stmt* body;
};

struct Break : Stmt { 
    Ident* label;
};

struct Continue : Stmt { 

};

// using path;
struct Include : Node { 
    std::vector<Ident*> path;
};

// using path(items);
struct MultiInclude : Include { 
    std::vector<Ident*> items;
};

struct Alias : Node { 
    Ident* name;
    Type* type;
};

struct Field : Node { 
    Ident* name;
    Type* type;
};

struct Record : Decl { 
    Ident* name;
    std::vector<Field*> fields;
};

struct Union : Decl { 
    Ident* name;
    std::vector<Field*> fields;
};

struct Case : Node { 
    Ident* name;
    Expr* value;
    std::vector<Field*> fields;
};

struct Variant : Decl { 
    Ident* name;
    Type* parent;
    std::vector<Case*> cases;
};

struct Param : Node {
    Ident* name;
    Type* type;
    Expr* value;
};

struct Function : Decl { 
    Ident* name;
    std::vector<Param*> params;
    Type* result;
};

// def name(args): type = expr;
struct SingleFunction : Function {
    Expr* expr;
};

// def name(args): type { body }
struct CompoundFunction : Function {
    Compound* body;
};

}

struct error {
    enum type {
        LEXER, // fowarding a lexer error

        EXPECT, // expected one token but got another
    };

    struct expected {
        token want;
        token got;
    };

    union data {
        expected expect;
    };

    error(type type, data data = {})
        : type(type)
        , data(data)
    { }

    type type;
    data data;
};

struct parser {
    parser(lexer* source);
    ~parser();

private:
    template<typename T, typename F>
    std::vector<T*> collect(key sep, F&& func) {
        std::vector<T*> out;

        do out.push_back(func()); while (eat(sep).valid());

        return out;
    }

    token eat(key key);
    token expect(token::kind type);
    token expect(key key);

    token next();
    token peek();

    token ahead;
    lexer* source;


    template<typename T, typename... A>
    T* make(A&&... args) {
        auto* out = new T(args...);
        nodes.push_back(out);
        return out;
    }

    std::vector<ast::Node*> nodes;
};

}

