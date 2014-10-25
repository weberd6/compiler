#ifndef SYMBOL_H
#define SYMBOL_H
#include <iostream>
#include <unordered_map>
#include <cstring>

enum type_t {
  // Single digit ASCII characters
  ZERO = 0,
  TOK_OPEN_PAREN = '(',
  TOK_CLOSE_PAREN = ')',
  TOK_MULTIPLY = '*',
  TOK_PLUS = '+',
  TOK_COMMA = ',',
  TOK_MINUS = '-',
  TOK_DIVIDE = '/',
  TOK_COLON = ':',
  TOK_SEMICOLON = ';',
  TOK_EQUALS = '=',
  TOK_OPEN_BRACE = '{',
  TOK_CLOSE_BRACE = '}',
  TOK_OPEN_BRACKET = '[',
  TOK_CLOSE_BRACKET = ']',
  TOK_AND = '&',
  TOK_OR = '|',
  
  // Multiple ASCII character symbols
  TOK_RELOP = 260,
  TOK_ASSIGNMENT = 261,
  
  //Reserved words
  RESERVED_FALSE = 267,
  RESERVED_TRUE = 268,
  RESERVED_IS = 269,
  RESERVED_STRING = 270,
  RESERVED_INT = 271,
  RESERVED_BOOL = 272,
  RESERVED_FLOAT = 273,
  RESERVED_GLOBAL = 274,
  RESERVED_IN = 275,
  RESERVED_OUT = 276,
  RESERVED_IF = 277,
  RESERVED_THEN = 278,
  RESERVED_ELSE = 279,
  RESERVED_CASE = 280,
  RESERVED_FOR = 281,
  RESERVED_AND = 282,
  RESERVED_OR = 283,
  RESERVED_NOT = 284,
  RESERVED_PROGRAM = 285,
  RESERVED_PROCEDURE = 286,
  RESERVED_BEGIN = 287,
  RESERVED_RETURN = 288,
  RESERVED_END = 289,
  
  // Identifiers, and types
  TOK_IDENTIFIER = 290,
  TYPE_INT = 291,
  TYPE_FLOAT = 292,
  TYPE_STRING = 294,
  TYPE_BOOL = 295,
  
  TOK_EOF = 310,
};

enum id_type_t {
  ID_VARIABLE,
  ID_PROCEDURE,
  ID_PARAMETER,
};

enum direction_t {
  DIRECTION_UNSPECIFIED,
  DIRECTION_IN,
  DIRECTION_OUT,
};

struct Type {
  public:
    Type(){ symbol = true; };
    Type(type_t t, bool array, unsigned int size);
      
    Type(const Type& type);
    Type& operator=(const Type& type);
    bool operator==(const Type& type);
    bool operator!=(const Type& type);

    bool array() { return is_array; }
    type_t type() { return variable_type; }
    unsigned int arraysize() { return array_size; }
    bool is_symbol() { return symbol; }

    void set_array(bool arr) { is_array = arr; }
    void set_type(type_t t) { variable_type = t; }
    void set_arraysize(unsigned int size) { array_size = size; }

  private:
    type_t variable_type;
    bool is_array;
    unsigned int array_size;
    bool symbol;
};

class Symbol {
  public:
    // Constructors
    Symbol(std::string nm, bool global, bool isarray, type_t typ, unsigned int size); // Variables
    Symbol(std::string nm, bool isarray, type_t typ, unsigned int size);      // Parameters
    Symbol(std::string nm, bool global);  // Procedures
    
    ~Symbol(){}
    
    std::string name;
    int line_declared;
    
    int ref_count; // For deleting
    bool used;
    bool initialized;
    id_type_t id_type;
    bool is_global;
    Type symbol_type;
    int address;
    int width;
    direction_t direction;

    Symbol* params; // To build linked list for params
    Symbol* next;
};

typedef std::unordered_map<std::string, Symbol*> symbol_map;
typedef std::unordered_map<std::string, Symbol*>::iterator symbol_map_iterator;

#endif