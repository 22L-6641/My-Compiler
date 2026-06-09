#pragma once

#include <vector>
#include <string>
#include <initializer_list>
#include <fstream>
#include "lexer.hpp"
#include "tac.hpp"




class Parser {
public:
    Parser(std::vector<TOKEN>, TACGenerator& tacgen);
    void programme();
    bool hasErrors() const;
    int getErrorCount() const;

private:

    std::vector<TOKEN> tokens;
    size_t index;
    int error_count = 0;

    //parse tree vars
    int depth;
    std::ofstream parse_tree_out;
    bool next_line=true;
    const std::string parse_tree_directory = "output";
    const std::string parse_tree_filename = "parse_tree.txt";

    TOKEN peek();
    bool match(const std::string&);
    bool match(TOKEN_CLASS);
    bool checkLexeme(const std::string&);
    bool isTypeStart();
    void advance();
    void reportError(const std::string&);
    void syncTo(std::initializer_list<std::string> lexemes);


    bool drawInStart(const std::string&);
    bool drawInEnd();
    void write(const std::string&);

    // TAC generator reference
    TACGenerator& tac;

    void programme_1();
    void type();
    void argList();
    void declaration();
    void stmt();
    void noIfStmt();
    void forStmt();
    void optExpr();
    void whileStmt();
    void ifStmt();
    void elsePart();
    void compStmt();
    void stmtList();
    void returnStmt();
    std::string expr();
    std::string expr_1();
    std::string rvalue();
    std::string rvalue_1(const std::string& left);
    std::string mag();
    std::string mag_1(const std::string& left);
    std::string term();
    std::string term_1(const std::string& left);
    void compare();
    std::string factor();
    std::string identifier();
    std::string number();
    void ioStmt();
    bool isIdentifierLike();
};