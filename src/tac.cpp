#include "tac.hpp"
#include <fstream>
#include <iomanip>

TACGenerator::TACGenerator() : temp_count(0), label_count(0) {}

std::string TACGenerator::newTemp() {
    ++temp_count;
    return "t" + std::to_string(temp_count);
}

std::string TACGenerator::newLabel() {
    ++label_count;
    return "L" + std::to_string(label_count);
}

void TACGenerator::emit(const std::string& op, const std::string& arg1, const std::string& arg2, const std::string& result) {
    instructions.push_back({op, arg1, arg2, result});
}

void TACGenerator::emitLabel(const std::string& label) {
    instructions.push_back({"label", "", "", label});
}

void TACGenerator::emitIfFalse(const std::string& cond, const std::string& label) {
    instructions.push_back({"ifFalse", cond, "", label});
}

void TACGenerator::emitAssign(const std::string& lhs, const std::string& rhs) {
    instructions.push_back({"=", rhs, "", lhs});
}

void TACGenerator::writeToFile(const std::string& path) const {
    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open()) return;
    for (const auto& i : instructions) {
        if (i.op == "label") {
            out << i.result << ":\n";
        } else if (i.op == "ifFalse") {
            out << "ifFalse " << i.arg1 << " goto " << i.result << "\n";
        } else if (i.op == "=") {
            out << i.result << " = " << i.arg1 << "\n";
        } else if (i.op.empty()) {
            // no-op
        } else {
            out << i.result << " = " << i.arg1 << " " << i.op << " " << i.arg2 << "\n";
        }
    }
    out.close();
}
