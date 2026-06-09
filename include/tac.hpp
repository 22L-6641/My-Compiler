#pragma once

#include <string>
#include <vector>

struct TACInstruction {
    std::string op;
    std::string arg1;
    std::string arg2;
    std::string result;
};

class TACGenerator {
public:
    TACGenerator();
    std::string newTemp();
    std::string newLabel();
    void emit(const std::string& op, const std::string& arg1, const std::string& arg2, const std::string& result);
    void emitLabel(const std::string& label);
    void emitIfFalse(const std::string& cond, const std::string& label);
    void emitAssign(const std::string& lhs, const std::string& rhs);
    void writeToFile(const std::string& path) const;
private:
    int temp_count;
    int label_count;
    std::vector<TACInstruction> instructions;
};
