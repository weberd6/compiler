#ifndef PARSER_H
#define PARSER_H

#include <cstdlib>
#include <stdlib.h>
#include <stack>
#include <unistd.h>
#include <sstream>

#include "symbol.h"
#include "scanner.h"
#include "error.h"
#include "codegenerator.h"

class Parser {
  public:
    Parser(std::string filename);
    ~Parser();

    void start();
    static void add_to_symbol_table(Symbol* sym);
    Symbol* get_symbol(std::string id);
    
    bool EOF_found;
    
  protected:
    void match(type_t t);
    void move();
    void find(type_t type);
    void syntax_error(std::string mesg);
    void type_match(Type* lhs_type, Type* rhs_type, int argnum);
    Type* arithmetic_typecheck(Type* lhs_type, Type* rhs_type, type_t op_type);
    void relation_typecheck(Type* lhs_type, Type* rhs_type, relative_op_t relop);
    
    void program();
    void return_after_runtime();
    void program_header();
    void program_body();
    void declarations_();
    void statements();
    void statements_();
    void declaration();
    void declaration_(bool global);
    void procedure_declaration(bool global, id_type_t idt);
    std::string procedure_header(bool global, id_type_t idt);
    Symbol* paramlist();
    void paramlist_(Symbol* prev_sym);
    Symbol* parameter();
    direction_t inout();
    void procedure_body(std::string name);
    Symbol* variable_declaration(bool global, id_type_t idt);
    bool array_size_brackets(unsigned int &array_size);
    type_t typemark();
    unsigned int arraysize();
    void statement();
    void statement_(Symbol* left);
    void assignment();
    bool array_expression();
    void if_statement();
    void follow_if(int lab);
    void loop_statement();
    void return_statement();
    void expression_(Type*& lhs_type, bool out_param);
    void arithop_(Type*& lhs_type, bool out_param);
    void relation_(Type*& lhs_type, bool out_param);
    void term_(Type*& lhs_type, bool out_param);
    std::string identifier();
    Type* expression(bool out_param);
    Type* arithop(bool out_param);
    Type* relation(bool out_param);
    Type* term(bool out_param);
    Type* factor(bool out_param);
    Type* factor_(bool out_param);
    void argument_list(Symbol* procedure);
    Symbol* argument_list_(Symbol* next_param, std::stack<Symbol*> &args, int &num_out_params);
    Type* name(bool out_param);
    Type* number(bool arraysize);
    Type* string();

  private:
    Token *next_token;
    Scanner *scanner;
    Code_generator *codegen;
    
    bool bad_out_param;
    
    char num[8];
    
    // Symbol table management
    static symbol_map_iterator map_iterator;
    static std::stack <symbol_map*> symbol_table;
    static symbol_map* global_symbol_map;
    static symbol_map* outer_symbol_map;
};

#endif