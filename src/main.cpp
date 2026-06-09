
#include <iostream>
#include <vector>
#include "lexer.hpp"
#include <parser.hpp>


int main(int argc, char* args[]) {

    if (argc == 1){
        std::cerr << "NO FILE SPECIFIED" << std::endl;
        exit(1);
    }



    clock_t start_time = clock(); 
    const char* input_filename = args[1];
    std::unordered_set<std::string> keywords = {
        "asm", "Wagarna", "new", "this", "auto", "enum", "operator", "throw", "Mantiqi",
        "explicit", "private", "True", "break", "export", "protected", "try", "case", 
        "extern", "public", "typedef", "catch", "False", "register", "typeid", "Harf", 
        "Ashriya", "typename", "Adadi", "class", "for", "Wapas", "union", "const", 
        "dost", "short", "unsigned", "goto", "signed", "using", "continue", "Agar", 
        "sizeof", "virtual", "default", "inline", "static", "Khali", "delete", 
        "volatile", "do", "long", "struct", "double", "mutable", "switch", "while", 
        "namespace", "template", "Marqazi", "Matn", "output->", "input<-"
    };
    
    
    TABLE<SYMBOL_TABLE_ENTRY> symbol_table;
    TABLE<LITERAL_TABLE_ENTRY> literal_table;
    std::vector<TOKEN> token_stream;

    Scanner(input_filename);

    
    double scanner_time = (double)(clock() - start_time) / CLOCKS_PER_SEC;
    std::cout <<"Scan complete. Time: " <<scanner_time << std::endl;
    

    Lexer lex((std::string(input_filename) + ".Meow").c_str(),symbol_table,literal_table,keywords);
    while (!lex.isEmpty())
    {
        token_stream.push_back(std::move(lex.getNextToken()));
    }


    std::cout << "\n\n--------TOKEN STREAM-------\n\n";
    for (const auto& token: token_stream) {
        std::cout << token.toString() << std::endl;
    }

    std::cout <<"Tokens generated. Time: " << (double)(clock() - start_time) / CLOCKS_PER_SEC - scanner_time << std::endl;
 

    
    symbol_table.writeToFile("output/symbol_table.txt");
    literal_table.writeToFile("output/literal_table.txt");
    std::cout << "----------Generated Lexer Output files---------\n";

    std::cout << "----------Parser----------\n";
    TACGenerator tac;
    Parser parser(token_stream, tac);

    parser.programme();

    if (parser.hasErrors()) {
        std::cerr << "There was error while parsing.\n";
        return EXIT_FAILURE;
    }

    // write TAC output
    tac.writeToFile("output/three_address_code.txt");

    std::cout << "Parsing completed successfully with 0 errors.\n";

    return EXIT_SUCCESS;    
}