#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include <stack>
#include <sstream>
#include "symbol.h"
#include "scanner.h"

class Code_generator {
  public:
    Code_generator(std::string filename);
    ~Code_generator();
    int next_label();
    void main_runtime();
    void start();
    void exit();
    void enter_procedure();
    void emit_procedure();
    void arithmetic_operation(type_t type, const char*);
    void relation_operation(relative_op_t relop);
    void invert();
    void change_sign(type_t type);
    void calculate_address(Symbol* sym);
    void calculate_array_element_address(Symbol* sym, bool top, bool addr_already, bool copy_index);
    void cast_to_float(bool top);
    int if_false(std::string labelname);
    int if_true(std::string labelname);
    void label(std::string labelname, int num);
    void goto_(std::string labelname, int num);
    void stack_alloc_param(Symbol* sym, bool in_param);
    void stack_alloc_local(Symbol* sym);
    void alloc_static(Symbol* sym);
    void reset_fp_address();
    void push_parameters(std::stack<Symbol*> &args, int num_out_params);
    void call_procedure(std::string name);
    void caller_return(Symbol* sym);
    void callee_return(bool runtime);
    void assignment(Symbol *left, bool is_int);
    void assign_indexed(Symbol* left, bool is_int);
    void get_bool_value(bool b);
    void get_string_literal(Token* next_token);
    void get_constant(bool is_int, Token* next_token);
    void get_indexed_value(bool is_int);
    void get_value(bool is_int, Symbol* sym);
    void spill_register();
    void valid_data_check(bool top);
    
  protected:
    void output_relop(relative_op_t relop);
    
  private:
   std::ofstream output_file;
   std::string rawname;
    
    std::stack <std::stringstream*> procedure_code;
    std::stack <int> reg_num_stack;
    
    int reg;
    
    int static_address;
    int label_num;
    int rel_fp_address;
  
};

#endif