#include "parser.hpp"
#include <iostream>
#include <cstdlib>

bool Parser::hasErrors() const {
    return error_count > 0;
}

int Parser::getErrorCount() const {
    return error_count;
}

Parser::Parser(std::vector<TOKEN> _tokens) : tokens(std::move(_tokens)), index(0), depth(0), next_line(false) {
    parse_tree_out.open(parse_tree_directory + "/" + parse_tree_filename, std::ios::out | std::ios::trunc);
    if (!parse_tree_out.is_open()) {
        std::cerr << "Parser:Parser() unable to open parse tree file\n";
    }
}

bool Parser::drawInStart(const std::string& node_name) {
    ++depth;
    next_line = true;
    write(node_name);
    return true;
}

bool Parser::drawInEnd() {
    if (next_line) write("");
    next_line = false;
    --depth;
    return true;
}

void Parser::write(const std::string& node_name) {
    if (!parse_tree_out.is_open()) {
        return;
    }

    std::string line;
    for (int i = 0; i < depth; ++i) line += '\t';
    if (!node_name.empty()) {
        line += node_name + "\n";
        for (int i = 0; i < depth; ++i) line += '\t';
        line += "|\n";
    } else {
        line += "\n";
    }
    parse_tree_out << line;
}

TOKEN Parser::peek() {
    if (index >= tokens.size()) {
        return TOKEN(-1, std::nullopt, TOKEN_CLASS::T_EOF, 0, 0);
    }
    return tokens[index];
}

bool Parser::checkLexeme(const std::string& lexeme) {
    return peek().t_lexeme.has_value() && peek().t_lexeme.value() == lexeme;
}

bool Parser::isTypeStart() {
    return checkLexeme("Adadi") || checkLexeme("Ashriya") || checkLexeme("Harf") || checkLexeme("Mantiqi") ||
           checkLexeme("Matn") || checkLexeme("Khali");
}

bool Parser::isIdentifierLike() {
    return peek().t_class == Identifier || checkLexeme("Marqazi");
}

void Parser::advance() {
    if (index < tokens.size()) {
        ++index;
    }
}

void Parser::reportError(const std::string& message) {
    ++error_count;
    std::cerr << "[PARSE ERROR] " << message << " at line " << peek().line_number << "\n";
}

void Parser::syncTo(std::initializer_list<std::string> lexemes) {
    while (peek().t_class != T_EOF) {
        for (const auto& lexeme : lexemes) {
            if (checkLexeme(lexeme)) {
                return;
            }
        }
        advance();
    }
}

bool Parser::match(const std::string& lexeme) {
    if (peek().t_lexeme == lexeme) {
        advance();
        return true;
    } else {
        reportError("Expected '" + lexeme + "'");
        if (peek().t_class != T_EOF) {
            advance();
        }
        return false;
    }
}

bool Parser::match(TOKEN_CLASS cls) {
    if (peek().t_class == cls) {
        advance();
        return true;
    } else {
        reportError("Expected token class");
        if (peek().t_class != T_EOF) {
            advance();
        }
        return false;
    }
}



void Parser::programme() {
    drawInStart("programme");
    if (peek().t_class != T_EOF) {
        if (!isTypeStart()) {
            reportError("Expected type at programme start");
            syncTo({"Adadi", "Ashriya", "Harf", "Mantiqi", "Matn", "Khali", "::", "("});
        }

        if (isTypeStart()) {
            type();
            if (isIdentifierLike()) {
                identifier();
            } else {
                reportError("Expected identifier after type");
                syncTo({"(", "::", "Adadi", "Ashriya", "Harf", "Mantiqi", "Matn", "Khali", "}"});
            }
            programme_1();
        }
    }
    drawInEnd();
}

void Parser::programme_1() {
    drawInStart("programme_1");
    if (checkLexeme("(")) {
        match("("); argList(); match(")"); compStmt(); programme();
    } else if (checkLexeme("::")) {
        match("::"); programme();
    } else if (peek().t_class != T_EOF) {
        reportError("Expected '(' or '::' after function identifier");
        syncTo({"::", "Adadi", "Ashriya", "Harf", "Mantiqi", "Matn", "Khali", "}"});
    }
    drawInEnd();
}

void Parser::type() {
    drawInStart("type");
    if (isTypeStart()) {
        advance();
    } else {
        reportError("Expected type");
        syncTo({"Adadi", "Ashriya", "Harf", "Mantiqi", "Matn", "Khali", "::", ",", ")", "}"});
    }
    drawInEnd();
}

void Parser::argList() {
    drawInStart("argList");
    if (isTypeStart()) {
        type(); identifier();
        if (checkLexeme(",")) {
            match(","); argList();
        }
    }
    drawInEnd();
}

void Parser::declaration() {
    drawInStart("declaration");
    if (!isTypeStart()) {
        reportError("Expected type in declaration");
        syncTo({"::", "}", "Wagarna"});
        drawInEnd();
        return;
    }
    type(); identifier();
    if (checkLexeme("=") || checkLexeme(":=")) {
        match(peek().t_lexeme.value()); expr();
    }
    while (checkLexeme(",")) {
        match(","); identifier();
        if (checkLexeme("=") || checkLexeme(":=")) {
            match(peek().t_lexeme.value()); expr();
        }
    }
    match("::");
    drawInEnd();
}

void Parser::stmt() {
    drawInStart("stmt");
    if (checkLexeme("Agar")) ifStmt();
    else noIfStmt();
    drawInEnd();
}

void Parser::noIfStmt() {
    drawInStart("noIfStmt");
    if (checkLexeme("for")) forStmt();
    else if (checkLexeme("while")) whileStmt();
    else if (checkLexeme("{")) compStmt();
    else if (checkLexeme("Wapas")) returnStmt();
    else if (checkLexeme("output<-") || checkLexeme("input->")) ioStmt();
    else if (isTypeStart()) declaration();
    else if (isIdentifierLike()) { expr(); match("::"); }
    else if (checkLexeme("::")) match("::");
    else {
        reportError("Invalid statement");
        syncTo({"::", "}", "Wagarna", "Agar", "for", "while", "{"});
        if (checkLexeme("::")) {
            match("::");
        } else if (checkLexeme("Wagarna")) {
            // Leave else marker for ifStmt/stmtList to handle.
        } else if (peek().t_class != T_EOF && !checkLexeme("}")) {
            // Guarantee forward progress during recovery.
            advance();
        }
    }
    drawInEnd();
}

void Parser::forStmt() {
    drawInStart("forStmt");
    match("for"); match("("); optExpr(); match("::"); optExpr(); match("::"); optExpr(); match(")"); stmt();
    drawInEnd();
}

void Parser::optExpr() {
    drawInStart("optExpr");
    if (isIdentifierLike() || checkLexeme("(") || peek().t_class == Number || peek().t_class == String_Literal ||
        checkLexeme("True") || checkLexeme("False")) expr();
    drawInEnd();
}

void Parser::whileStmt() {
    drawInStart("whileStmt");
    match("while"); match("("); expr(); match(")"); stmt();
    drawInEnd();
}

void Parser::ifStmt() {
    drawInStart("ifStmt");
    match("Agar"); match("("); expr(); match(")");
    stmt();
    if (checkLexeme("Wagarna")){
        elsePart();
    }
    drawInEnd();
}
void Parser::elsePart() {
    drawInStart("elsePart");
    match("Wagarna");
    stmt();
    drawInEnd();
}

void Parser::compStmt() {
    drawInStart("compStmt");
    match("{"); stmtList(); match("}");
    drawInEnd();
}

void Parser::stmtList() {
    drawInStart("stmtList");
    if (!checkLexeme("}") && !checkLexeme("Wagarna") && peek().t_class != T_EOF) {
        stmt(); stmtList();
    }
    drawInEnd();
}

void Parser::returnStmt() {
    drawInStart("returnStmt");
    match("Wapas"); expr(); match("::");
    drawInEnd();
}

void Parser::expr() {
    drawInStart("expr");
    if (isIdentifierLike()) {
        identifier(); expr_1();
    } else if (peek().t_class == Number || peek().t_class == String_Literal || checkLexeme("True") || checkLexeme("False") || checkLexeme("(")) {
        rvalue();
    } else {
        reportError("Invalid expression start");
        syncTo({"::", ")", "}", "Wagarna", ","});
    }
    drawInEnd();
}

void Parser::expr_1() {
    drawInStart("expr_1");
    if (checkLexeme(":=")) {
        match(":="); expr();
    } else {
        rvalue_1();
    }
    drawInEnd();
}

void Parser::rvalue() {
    drawInStart("rvalue");
    mag(); rvalue_1();
    drawInEnd();
}

void Parser::rvalue_1() {
    drawInStart("rvalue_1");
    if (checkLexeme("==") || checkLexeme("<") || checkLexeme(">") ||
        checkLexeme("<=") || checkLexeme(">=") || checkLexeme("!=") ||
        checkLexeme("<>")) {
        compare(); mag(); rvalue_1();
    }
    drawInEnd();
}

void Parser::mag() {
    drawInStart("mag");
    term(); mag_1();
    drawInEnd();
}

void Parser::mag_1() {
    drawInStart("mag_1");
    if (checkLexeme("+") || checkLexeme("-")) {
        match(peek().t_lexeme.value()); term(); mag_1();
    }
    drawInEnd();
}

void Parser::term() {
    drawInStart("term");
    factor(); term_1();
    drawInEnd();
}

void Parser::term_1() {
    drawInStart("term_1");
    if (checkLexeme("*") || checkLexeme("/")) {
        match(peek().t_lexeme.value()); factor(); term_1();
    }
    drawInEnd();
}

void Parser::compare() {
    drawInStart("compare");
    match(peek().t_lexeme.value());
    drawInEnd();
}

void Parser::factor() {
    drawInStart("factor");
    if (checkLexeme("(")) {
        match("("); expr(); match(")");
    } else if (isIdentifierLike()) {
        identifier();
    } else if (peek().t_class == Number || peek().t_class == String_Literal || checkLexeme("True") || checkLexeme("False")) {
        number();
    } else {
        reportError("Invalid factor");
        syncTo({")", "::", "+", "-", "*", "/", "==", "<", ">", "<=", ">=", "!=", "<>", "}", "Wagarna"});
    }
    drawInEnd();
}

void Parser::identifier() {
    drawInStart("Identifier");
    if (peek().t_class == Identifier) {
        match(Identifier);
    } else if (checkLexeme("Marqazi")) {
        match("Marqazi");
    } else {
        reportError("Expected identifier");
        if (peek().t_class != T_EOF) {
            advance();
        }
    }
    drawInEnd();
}

void Parser::number() {
    drawInStart("Number");
    if (peek().t_class == Number || peek().t_class == String_Literal) {
        match(peek().t_class);
    } else if (checkLexeme("True") || checkLexeme("False")) {
        match(peek().t_lexeme.value());
    } else {
        reportError("Expected literal");
        if (peek().t_class != T_EOF) {
            advance();
        }
    }
    drawInEnd();
}

void Parser::ioStmt() {
    drawInStart("ioStmt");
    if (checkLexeme("output<-")) {
        match("output<-");
        expr();
        match("::");
    } else if (checkLexeme("input->")) {
        match("input->");
        identifier();
        match("::");
    } else {
        reportError("Expected input/output statement");
        if (peek().t_class != T_EOF) {
            advance();
        }
    }
    drawInEnd();
}