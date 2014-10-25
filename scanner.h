#ifndef SCANNER_H
#define SCANNER_H

#include <fstream>

#include "symbol.h"

enum relative_op_t {
  IS_EQUAL,
  NOT_EQUAL,
  GREATER_THAN,
  LESS_THAN,
  GREATER_OR_EQUAL,
  LESS_OR_EQUAL,
};

class Token {
 public:
    Token(type_t t){ type = t; }
    
    type_t type;
    union value_t {
      int int_value;
      double float_value;
      relative_op_t relop;
    } value;
    std::string string;
};

typedef std::unordered_map<std::string, type_t>::iterator word_iterator;
typedef std::unordered_map<std::string, type_t> word_map;

class Scanner {
  public:
    Scanner(std::string filename);
    ~Scanner();
  
    bool init(std::string filename);
    
    static void report_warning(std::string message);
    void syntax_error(std::string mesg);
    
    static std::string& print_token(type_t type, std::string& string);
    static std::ifstream input_file;
    static int line_number;
    static int num_errors;
    
    static bool warnings;
    static int num_warnings;
  
    Token* get_token(std::ifstream& ifs);
    
  protected:
    void reserve(type_t type, std::string word);
    bool is_reserved(std::string word);
    type_t get_reserved_type(std::string word);
  
  private:
    std::string file_name;
    
    word_map reserved_words;
    word_iterator reserved_words_iterator;
};

#endif // SCANNER_H