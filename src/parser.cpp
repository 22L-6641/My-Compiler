#include "parser.hpp"
#include <iostream>
#include <cstdlib>

bool Parser::hasErrors() const {
    return error_count > 0;
}

int Parser::getErrorCount() const {
    return error_count;
}

Parser::Parser(std::vector<TOKEN> _tokens, TACGenerator& tacgen) : tokens(std::move(_tokens)), index(0), depth(0), next_line(false), tac(tacgen) {
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
    match("for"); match("(");
    // init
    if (!checkLexeme("::")) {
        expr();
    }
    match("::");
    // cond
    std::string cond;
    if (!checkLexeme("::")) {
        cond = expr();
    }
    match("::");
    // incr
    std::streampos incr_pos; // not needed explicitly
    std::string startLabel = tac.newLabel();
    std::string endLabel = tac.newLabel();
    tac.emitLabel(startLabel);
    if (!cond.empty()) {
        tac.emitIfFalse(cond, endLabel);
    }
    match(")");
    stmt();
    // increment: attempt to parse an expression if present (best-effort)
    // Note: original grammar allowed optExpr; here we won't reparse tokens after stmt.
    tac.emit("goto", "", "", startLabel);
    tac.emitLabel(endLabel);
    drawInEnd();
}

void Parser::optExpr() {
    drawInStart("optExpr");
    if (isIdentifierLike() || checkLexeme("(") || peek().t_class == Number || peek().t_class == String_Literal ||
        checkLexeme("True") || checkLexeme("False")) {
        expr();
    }
    drawInEnd();
}

void Parser::whileStmt() {
    drawInStart("whileStmt");
    match("while"); match("(");
    std::string start = tac.newLabel();
    std::string end = tac.newLabel();
    tac.emitLabel(start);
    std::string cond = expr();
    match(")");
    tac.emitIfFalse(cond, end);
    stmt();
    tac.emit("goto", "", "", start);
    tac.emitLabel(end);
    drawInEnd();
}

void Parser::ifStmt() {
    drawInStart("ifStmt");
    match("Agar"); match("(");
    std::string cond = expr();
    match(")");
    std::string elseLabel = tac.newLabel();
    std::string endLabel = tac.newLabel();
    tac.emitIfFalse(cond, elseLabel);
    stmt();
    if (checkLexeme("Wagarna")){
        tac.emit("goto", "", "", endLabel);
        tac.emitLabel(elseLabel);
        elsePart();
        tac.emitLabel(endLabel);
    } else {
        tac.emitLabel(elseLabel);
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

std::string Parser::expr() {
    drawInStart("expr");
    std::string result;
    if (isIdentifierLike()) {
        std::string id = identifier();
        if (checkLexeme(":=")) {
            match(":=");
            std::string rhs = expr();
            if (!rhs.empty()) tac.emitAssign(id, rhs);
            result = id;
        } else {
            // Parse full precedence chain when expression starts with identifier.
            std::string termRes = term_1(id);
            std::string current = termRes.empty() ? id : termRes;

            std::string magRes = mag_1(current);
            current = magRes.empty() ? current : magRes;

            std::string relRes = rvalue_1(current);
            result = relRes.empty() ? current : relRes;
        }
    } else if (peek().t_class == Number || peek().t_class == String_Literal || checkLexeme("True") || checkLexeme("False") || checkLexeme("(")) {
        result = rvalue();
    } else {
        reportError("Invalid expression start");
        syncTo({"::", ")", "}", "Wagarna", ","});
    }
    drawInEnd();
    return result;
}

std::string Parser::expr_1() {
    drawInStart("expr_1");
    std::string res;
    if (checkLexeme(":=")) {
        match(":=");
        res = expr();
    } else {
        res = rvalue();
    }
    drawInEnd();
    return res;
}

std::string Parser::rvalue() {
    drawInStart("rvalue");
    std::string left = mag();
    std::string res = rvalue_1(left);
    drawInEnd();
    return res.empty() ? left : res;
}

std::string Parser::rvalue_1(const std::string& left) {
    drawInStart("rvalue_1");
    std::string res;
    if (checkLexeme("==") || checkLexeme("<") || checkLexeme(">") ||
        checkLexeme("<=") || checkLexeme(">=") || checkLexeme("!=") ||
        checkLexeme("<>")) {
        std::string op = peek().t_lexeme.value();
        compare();
        std::string right = mag();
        std::string temp = tac.newTemp();
        tac.emit(op, left, right, temp);
        res = rvalue_1(temp);
        if (res.empty()) res = temp;
    }
    drawInEnd();
    return res;
}

std::string Parser::mag() {
    drawInStart("mag");
    std::string left = term();
    std::string res = mag_1(left);
    drawInEnd();
    return res.empty() ? left : res;
}

std::string Parser::mag_1(const std::string& left) {
    drawInStart("mag_1");
    std::string res;
    if (checkLexeme("+") || checkLexeme("-")) {
        std::string op = peek().t_lexeme.value();
        match(op);
        std::string right = term();
        std::string temp = tac.newTemp();
        tac.emit(op, left, right, temp);
        res = mag_1(temp);
        if (res.empty()) res = temp;
    }
    drawInEnd();
    return res;
}

std::string Parser::term() {
    drawInStart("term");
    std::string left = factor();
    std::string res = term_1(left);
    drawInEnd();
    return res.empty() ? left : res;
}

std::string Parser::term_1(const std::string& left) {
    drawInStart("term_1");
    std::string res;
    if (checkLexeme("*") || checkLexeme("/")) {
        std::string op = peek().t_lexeme.value();
        match(op);
        std::string right = factor();
        std::string temp = tac.newTemp();
        tac.emit(op, left, right, temp);
        res = term_1(temp);
        if (res.empty()) res = temp;
    }
    drawInEnd();
    return res;
}

void Parser::compare() {
    drawInStart("compare");
    match(peek().t_lexeme.value());
    drawInEnd();
}

std::string Parser::factor() {
    drawInStart("factor");
    std::string res;
    if (checkLexeme("(")) {
        match("(");
        res = expr();
        match(")");
    } else if (isIdentifierLike()) {
        res = identifier();
    } else if (peek().t_class == Number || peek().t_class == String_Literal || checkLexeme("True") || checkLexeme("False")) {
        res = number();
    } else {
        reportError("Invalid factor");
        syncTo({")", "::", "+", "-", "*", "/", "==", "<", ">", "<=", ">=", "!=", "<>", "}", "Wagarna"});
    }
    drawInEnd();
    return res;
}

std::string Parser::identifier() {
    drawInStart("Identifier");
    std::string name;
    if (peek().t_class == Identifier) {
        if (peek().t_lexeme.has_value()) name = peek().t_lexeme.value();
        match(Identifier);
    } else if (checkLexeme("Marqazi")) {
        name = "Marqazi";
        match("Marqazi");
    } else {
        reportError("Expected identifier");
        if (peek().t_class != T_EOF) {
            advance();
        }
    }
    drawInEnd();
    return name;
}

std::string Parser::number() {
    drawInStart("Number");
    std::string val;
    if (peek().t_class == Number || peek().t_class == String_Literal) {
        if (peek().t_lexeme.has_value()) val = peek().t_lexeme.value();
        match(peek().t_class);
    } else if (checkLexeme("True") || checkLexeme("False")) {
        if (peek().t_lexeme.has_value()) val = peek().t_lexeme.value();
        match(peek().t_lexeme.value());
    } else {
        reportError("Expected literal");
        if (peek().t_class != T_EOF) {
            advance();
        }
    }
    drawInEnd();
    return val;
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