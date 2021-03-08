#pragma once

#include <parser.hpp>
#include "tlexer.hpp"

using namespace ast;

struct TestParser : Parser {
    TestParser(TestLexer* lexer) : Parser(lexer), lex(lexer) { }

    void finish() { lex->expect(Token::END); }

    template<typename F>
    void expect(F&& func, ptr<ast::Node> node) {
        ptr<ast::Node> temp = func();
        ASSERT(temp);
        ASSERT(node);
        if (!temp->equals(node)) {
            Printer lhs;
            Printer rhs;
            temp->visit(&lhs);
            node->visit(&rhs);

            fprintf(stderr, "expected:\n%s\n", rhs.buffer.c_str());
            fprintf(stderr, "actual:\n%s\n", lhs.buffer.c_str());
            exit(1);
        }
    }

private:
    TestLexer* lex;
};
