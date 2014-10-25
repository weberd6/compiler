#include "parser.h"
#include "error.h"

symbol_map_iterator Parser::map_iterator;
std::stack <symbol_map*> Parser::symbol_table;
symbol_map* Parser::global_symbol_map;
symbol_map* Parser::outer_symbol_map;

Parser::Parser(std::string filename)
{    
  EOF_found = false;
  
  // Create hash maps for symbol lookup
  global_symbol_map = new symbol_map;
  outer_symbol_map = new symbol_map;
  // Push outer scope hash map into symbol table
  symbol_table.push(outer_symbol_map);
  // Create scanner
  scanner = new Scanner(filename);
  // Create generator
  codegen = new Code_generator(filename);

  bad_out_param = false;
}
   
Parser::~Parser()
{
  delete scanner;
  delete codegen;

  // Delete everything else from global symbol table
  for (map_iterator = global_symbol_map->begin(); map_iterator != global_symbol_map->end(); map_iterator++) {
    map_iterator->second->ref_count--;
    if (map_iterator->second->id_type == ID_VARIABLE)
      delete map_iterator->second;
    else if (map_iterator->second->id_type == ID_PROCEDURE && !map_iterator->second->ref_count) {
      Symbol* sym = map_iterator->second->params;
      while (sym != NULL) {
        Symbol* temp = sym;
        sym = sym->next;
        delete temp;
      }
      delete map_iterator->second;
    }
  }
  delete global_symbol_map; // Delete global map
  
  symbol_map* smap = symbol_table.top();
  while (!symbol_table.empty()) {
    for (map_iterator = smap->begin(); map_iterator != smap->end(); map_iterator++) {
      map_iterator->second->ref_count--;
      if (map_iterator->second->id_type == ID_VARIABLE)
        delete map_iterator->second;
      else if (map_iterator->second->id_type == ID_PROCEDURE && !map_iterator->second->ref_count) {
        Symbol* sym = map_iterator->second->params;
        while (sym != NULL) {
          Symbol* temp = sym;
          sym = sym->next;
          delete temp;
        }
        delete map_iterator->second;
      }
    }
    delete symbol_table.top();
    symbol_table.pop();
    if (!symbol_table.empty())
      smap = symbol_table.top();
  }
}

void Parser::start()
{
  try {
    next_token = scanner->get_token(Scanner::input_file);
    program();
  }
  catch (Error& e) {
    std::cerr << e.what();
  }
}

void Parser::match(type_t t)
{  
  if (next_token->type == t) {
    move();
  }
  else {
    // If one of the sync tokens, throw and catch error and keep parsing
    if (t == RESERVED_IS || t == RESERVED_BEGIN || t == RESERVED_END || t == TOK_SEMICOLON
	|| t == TOK_OPEN_PAREN || t == TOK_CLOSE_PAREN || t == TOK_CLOSE_BRACKET || t == TOK_ASSIGNMENT || t == TOK_COMMA)
    {
      try { throw SyntaxError("Unexpected token, ", t, next_token->type); }
      catch (SyntaxError& e) { std::cerr << e.what();}
    }
    else // Something else will catch this error and a sync token will be found 
      throw SyntaxError("Unexpected token, ", t, next_token->type);
  }
}

void Parser::move()
{  
  if (next_token->type == TOK_EOF) {
    delete next_token;
    EOF_found = true;
  }
  else {
    delete next_token;
    next_token = scanner->get_token(Scanner::input_file);
  }
}

void Parser::find(type_t type)
{  
  while (true) {
    if (next_token->type == TOK_EOF)
      break;
    else if (next_token->type == type)
      break;
    else
      next_token = scanner->get_token(Scanner::input_file);
  }
}

void Parser::program()
{
  try { program_header(); }
  catch (Error& e) { std::cerr << e.what(); std::cout << "Fatal error, compilation terminated\n"; exit(1); }

  codegen->main_runtime();

  try { program_body(); }
  catch (Error& e) { std::cerr << e.what(); std::cout << "Compilation terminated\n"; find(TOK_EOF); exit(1);}
  
  for (map_iterator = global_symbol_map->begin(); map_iterator != global_symbol_map->end(); map_iterator++) {
    if (!map_iterator->second->used && map_iterator->second->id_type == ID_VARIABLE)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Unused variable '"+map_iterator->second->name+"'\n");
    if (!map_iterator->second->initialized && map_iterator->second->id_type == ID_VARIABLE)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Uninitialized variable '"+map_iterator->second->name+"'\n");
  }
  
  for (map_iterator = outer_symbol_map->begin(); map_iterator != outer_symbol_map->end(); map_iterator++) {
    if (!map_iterator->second->used && map_iterator->second->id_type == ID_VARIABLE)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Unused variable '"+map_iterator->second->name+"'\n");
    if (!map_iterator->second->initialized && map_iterator->second->id_type == ID_VARIABLE)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Uninitialized variable '"+map_iterator->second->name+"'\n");
  }
    
  codegen->exit();
  
}

void Parser::program_header()
{
  match(RESERVED_PROGRAM);
  try { identifier(); }
  catch (Error& e){ std::cerr << e.what(); find(RESERVED_IS); }
  match(RESERVED_IS);
}

void Parser::program_body()
{
  type_t type = next_token->type;
  if (type == RESERVED_GLOBAL || type == RESERVED_PROCEDURE || type == RESERVED_INT
      || type == RESERVED_FLOAT || type == RESERVED_BOOL || type == RESERVED_STRING ) {
    
    codegen->enter_procedure();
      
    try { declarations_(); }
    catch (Error& e) { std::cerr << e.what() ; find(RESERVED_BEGIN); }
    match(RESERVED_BEGIN);

    if (!Scanner::num_errors) {
      codegen->start();
      codegen->emit_procedure();
    }

    codegen->enter_procedure();

    try { statements_(); }
    catch (Error& e) { std::cerr << e.what(); find(RESERVED_END); }
    match(RESERVED_END);
    match(RESERVED_PROGRAM);

    if (!Scanner::num_errors)
      codegen->emit_procedure();
    
    match(TOK_EOF);
  }
}

void Parser::declarations_()
{
  while (true) {
    type_t type = next_token->type;
    if (type == RESERVED_GLOBAL || type == RESERVED_PROCEDURE || type == RESERVED_INT
	|| type == RESERVED_BOOL || type == RESERVED_FLOAT || type == RESERVED_STRING)
    {
      try { declaration(); }
      catch (Error& e) { std::cerr << e.what(); find(TOK_SEMICOLON); }
      match(TOK_SEMICOLON);
      continue;
    }
    else
      break;	// e production
  }
}

void Parser::statements()
{
  try { statement(); }
  catch (Error& e) { std::cerr << e.what(); find(TOK_SEMICOLON); }
  match(TOK_SEMICOLON);
  statements_();
}

void Parser::statements_()
{
  while (true) {
    type_t type = next_token->type;
    if (type == TOK_IDENTIFIER || type == RESERVED_IF || type == RESERVED_FOR || type == RESERVED_RETURN) {
      try { statement(); }
      catch (Error& e) { std::cerr << e.what(); find(TOK_SEMICOLON); }
      match(TOK_SEMICOLON);
      continue;
    }
    else
      break;	// e production
  }
}

void Parser::declaration()
{
  type_t type = next_token->type;
  if (type == RESERVED_GLOBAL) {
    match(RESERVED_GLOBAL); 
    declaration_(true);
  }
  else if (type == RESERVED_PROCEDURE || type == RESERVED_INT || type == RESERVED_FLOAT
	    || type == RESERVED_STRING || type == RESERVED_BOOL) {
    declaration_(false);
  }
  else { // error, not allowed
      throw SyntaxError("A declaration is expected.", ZERO, ZERO);
  }
}

void Parser::declaration_(bool global)
{
  type_t type = next_token->type;
  if (type == RESERVED_PROCEDURE) {
    procedure_declaration(global, ID_PROCEDURE);
  }
  else if (type == RESERVED_INT || type == RESERVED_FLOAT || type == RESERVED_STRING || type == RESERVED_BOOL) {
    variable_declaration(global, ID_VARIABLE);
  }
  else { // error, not allowed
      throw SyntaxError("A declaration is expected.", ZERO, ZERO);
  }
}

void Parser::procedure_declaration(bool global, id_type_t idt)
{
  codegen->reset_fp_address();
  std::string name = procedure_header(global, idt);
  
  codegen->enter_procedure();
  codegen->label(name, -1);

  codegen->reset_fp_address();
  procedure_body(name);
  
  if (!Scanner::num_errors)
    codegen->emit_procedure();
}

std::string Parser::procedure_header(bool global, id_type_t idt)
{
  Symbol* new_symbol = NULL;
  match(RESERVED_PROCEDURE);
  
  std::string id;
  try { id = identifier(); }
  catch (Error& e) { std::cerr << e.what(); find(TOK_OPEN_PAREN); }
  
  // Create symbol for procedure if valid id
  if (!id.empty()) {
    new_symbol = new Symbol(id, global);
  }
  
  // Add procedure to scope its declared in
  if (new_symbol)
    add_to_symbol_table(new_symbol);
  
  match(TOK_OPEN_PAREN);
  
  // Always add new scope when we expect a procedure declaration
  symbol_map *map = new symbol_map;
  symbol_table.push(map);
  
  Symbol* params;
  try { params = paramlist(); }
  catch (Error& e) { std::cerr << e.what(); find(TOK_CLOSE_PAREN); }
  match(TOK_CLOSE_PAREN);
  
  // Add procedure to its own scope so it can be recursive if not global and add paramlist
  if (new_symbol && !(new_symbol->is_global)) {
    add_to_symbol_table(new_symbol);
    new_symbol->params = params;
  }
  else if (new_symbol)
    new_symbol->params = params;
  
  return id;
}

Symbol* Parser::paramlist()
{
  type_t type = next_token->type;
  Symbol* first_param = NULL;
  if (type == RESERVED_INT || type == RESERVED_FLOAT || type == RESERVED_STRING || type == RESERVED_BOOL) {
    first_param = parameter();
    paramlist_(first_param);
  }
  return first_param;
}

void Parser::paramlist_(Symbol* first_param)
{
  Symbol* curr_sym = first_param;
  while (true) {
    type_t type = next_token->type;
    
    if (type == TOK_COMMA) {
      match(TOK_COMMA); 
      curr_sym->next = parameter();
      curr_sym = curr_sym->next;
      continue;
    }
    else
      break;	// e production
  }
}

Symbol* Parser::parameter()
{
  type_t type = next_token->type;
  Symbol* sym = NULL;
  if (type == RESERVED_INT || type == RESERVED_FLOAT || type == RESERVED_BOOL || type == RESERVED_STRING) {
    sym = variable_declaration(false, ID_PARAMETER);
    if (sym)
      sym->direction = inout();
    
    if (sym->direction == DIRECTION_IN)
      codegen->stack_alloc_param(sym, true);
    else
      codegen->stack_alloc_param(sym, false);
  }
  return sym;
}

direction_t Parser::inout()
{
  type_t type = next_token->type;
  if (type == RESERVED_IN) {
    match(RESERVED_IN);
    return DIRECTION_IN;
  }
  else if (type == RESERVED_OUT) {
    match(RESERVED_OUT);
    return DIRECTION_OUT;
  }
  else { // error, not allowed
    throw SyntaxError("Missing in/out specifier in procedure declaration.", ZERO, ZERO);
  }
}

void Parser::procedure_body(std::string name)
{
  try { declarations_(); }
  catch (Error& e) { std::cerr << e.what(); find(RESERVED_BEGIN); }
  match(RESERVED_BEGIN);
  
  try { statements_(); }
  catch (Error& e) { std::cerr << e.what(); find(RESERVED_END); }
  match(RESERVED_END);
  match(RESERVED_PROCEDURE);
  
  codegen->callee_return(false);
  // Delete procedure scope now that we have found end of procedure
  for (map_iterator = symbol_table.top()->begin(); map_iterator != symbol_table.top()->end(); map_iterator++) {
    if (!map_iterator->second->used && map_iterator->second->id_type == ID_VARIABLE)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Unused variable '"+map_iterator->second->name+"'\n");
    if (!map_iterator->second->initialized && map_iterator->second->id_type == ID_VARIABLE)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Uninitialized variable '"+map_iterator->second->name+"'\n");
    if (!map_iterator->second->used && map_iterator->second->id_type == ID_PARAMETER && map_iterator->second->direction == DIRECTION_IN)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Unused 'in' parameter '"+map_iterator->second->name+"'\n");
    if (!map_iterator->second->initialized && map_iterator->second->id_type == ID_PARAMETER && map_iterator->second->direction == DIRECTION_OUT)
      Scanner::report_warning("Line "+std::to_string(map_iterator->second->line_declared)+": Unitialized 'out' parameter '"+map_iterator->second->name+"'\n");
    
    map_iterator->second->ref_count--;
    if (map_iterator->second->id_type == ID_VARIABLE)
      delete map_iterator->second;
    else if (map_iterator->second->id_type == ID_PROCEDURE && !map_iterator->second->ref_count) {
      Symbol* sym = map_iterator->second->params;
      while (sym != NULL) {
        Symbol* temp = sym;
        sym = sym->next;
        delete temp;
      }
      delete map_iterator->second;
    }
  }
  delete symbol_table.top();
  symbol_table.pop();
}

Symbol* Parser::variable_declaration(bool global, id_type_t idt)
{
  // Get variable type and match
  type_t var_type = typemark(); 
  
  // Get token for identifier and match
  std::string id = identifier();
  
  // Get array size if array
  unsigned int array_size = 1;
  bool is_array = array_size_brackets(array_size);
  
  // Create symbol table entry if a valid id was found
  Symbol* new_symbol = NULL;
  if (!id.empty() && idt == ID_VARIABLE) {
    if (global) { // Static data
      new_symbol = new Symbol(id, global, is_array, var_type, array_size);
      codegen->alloc_static(new_symbol);
      new_symbol->line_declared = Scanner::line_number;
    }
    else { // Allocate space on stack, local data
      new_symbol = new Symbol(id, global, is_array, var_type, array_size);
      codegen->stack_alloc_local(new_symbol);
      new_symbol->line_declared = Scanner::line_number;
    }
  }
  else if (!id.empty() && idt == ID_PARAMETER) {
    new_symbol = new Symbol(id, is_array, var_type, array_size);
    new_symbol->line_declared = Scanner::line_number;
  }
  
  // Add variable to current scope.
  if (new_symbol)
    add_to_symbol_table(new_symbol);
  
  return new_symbol;
}

bool Parser::array_size_brackets(unsigned int &array_size)
{
  type_t type = next_token->type;
  if (type == TOK_OPEN_BRACKET) {
    match(TOK_OPEN_BRACKET);
    // Array size must be an int, if not find bracket close
    try { array_size = arraysize(); }
    catch (Error& e) { std::cerr << e.what(); find(TOK_CLOSE_BRACKET);}
    match(TOK_CLOSE_BRACKET);
    return true;
  }
  return false;
}

type_t Parser::typemark()
{
  type_t type = next_token->type;
  type_t ret;
  if (type == RESERVED_INT) {
    match(RESERVED_INT);
    ret = TYPE_INT;
  }
  else if (type == RESERVED_FLOAT) {
    match(RESERVED_FLOAT);
    ret = TYPE_FLOAT;
  }
  else if (type == RESERVED_STRING) {
    match(RESERVED_STRING);
    ret = TYPE_STRING;
  }
  else if (type == RESERVED_BOOL) {
    match(RESERVED_BOOL);
    ret =  TYPE_BOOL;
  }
  else { // error, not allowed
    throw SyntaxError("Missing type identifier in procedure declaration.", ZERO, ZERO);
  }
  return ret;
}

unsigned int Parser::arraysize()
{
  unsigned int array_size = 0;
  if(next_token->type == TYPE_INT) {
    array_size = next_token->value.int_value;
    Type* type = number(true);
    if (type && !type->is_symbol()) delete type;
  }
  else
    throw Error("Array size must be an integer");
  return array_size;
}

void Parser::statement()
{
  type_t type = next_token->type;
  if (type == TOK_IDENTIFIER) {
    std::string id = identifier();
    Symbol* sym = get_symbol(id);
    statement_(sym);
  }
  else if (type == RESERVED_IF) {
    if_statement();
  }
  else if (type == RESERVED_FOR) {
    loop_statement();
  }
  else if (type == RESERVED_RETURN) {
    return_statement();
  }
  else  { // error, not allowed
      throw SyntaxError("There must be at least one statement within an if clause.", ZERO, ZERO);
  }
}

void Parser::statement_(Symbol* left)
{
  Type* rhs_type = NULL;
  type_t type = next_token->type;
  if (type == TOK_OPEN_BRACKET) {
    array_expression();
    match(TOK_ASSIGNMENT);
    rhs_type = expression(false);
    
    Type* type = new Type(left->symbol_type.type(), false, 1);
    type_match(type, rhs_type, 0);
    delete type;
    
    codegen->calculate_array_element_address(left, false, false, false);
    
    if (rhs_type->type() != TYPE_FLOAT)
      codegen->assign_indexed(left, true);
    else
      codegen->assign_indexed(left, false);
    
    if(rhs_type && !rhs_type->is_symbol()) delete rhs_type;
    
    left->initialized = true;
    if (left->direction == DIRECTION_IN)
      Scanner::report_warning("Line "+std::to_string(Scanner::line_number)+": An in parameter is being modified\n");
  }
  else if (type == TOK_OPEN_PAREN) {
    match(TOK_OPEN_PAREN);
    try { argument_list(left); }
    catch (Error& e) { std::cerr << e.what(); find(TOK_CLOSE_PAREN); }
    match(TOK_CLOSE_PAREN);
    codegen->call_procedure(left->name);
    codegen->caller_return(left->params);
  }
  else { // Must be empty array expression
    match(TOK_ASSIGNMENT);
    rhs_type = expression(false);
    type_match(&left->symbol_type, rhs_type, 0);
    
    if (rhs_type->type() != TYPE_FLOAT)
      codegen->assignment(left, true);
    else
      codegen->assignment(left, false);
    
    if (rhs_type && !rhs_type->is_symbol()) delete rhs_type;
    
    left->initialized = true;
    if (left->direction == DIRECTION_IN)
      Scanner::report_warning("Line "+std::to_string(Scanner::line_number)+": An in parameter is being modified\n");
  }
}

void Parser::assignment() // This assignment is used in loops, can be empty
{
  if (next_token->type == TOK_IDENTIFIER) {
    std::string lhs = identifier();
    bool arr_exp = array_expression();
    match(TOK_ASSIGNMENT);
    Type* rhs_type = expression(false);

    Symbol* lhs_sym = get_symbol(lhs);
    
    Type* type = new Type(lhs_sym->symbol_type.type(), false, 1);
    type_match(type, rhs_type, 0);
    delete type;
    
    if (lhs_sym->symbol_type.array() && arr_exp) {
      codegen->calculate_array_element_address(lhs_sym, true, false, false);
      if (rhs_type->type() != TYPE_FLOAT)
        codegen->assign_indexed(lhs_sym, true);
      else
        codegen->assign_indexed(lhs_sym, false);
    }
    // Is an array but no brackets
    else if (lhs_sym->symbol_type.array()) {
      throw Error("Invalid array assignment.\n");
    }
    // Has brackets but not an array
    else if (arr_exp) {
      throw Error("Invalid use of [] in non-array type.\n");
    }
    else {
      if (rhs_type->type() != TYPE_FLOAT)
        codegen->assignment(lhs_sym, true);
      else
        codegen->assignment(lhs_sym, false);
    }
    if (rhs_type && !rhs_type->is_symbol()) delete rhs_type;
    
    lhs_sym->initialized = true;
    if (lhs_sym->direction == DIRECTION_IN)
      Scanner::report_warning("Line "+std::to_string(Scanner::line_number)+": An in parameter is being modified\n");
  }
}

bool Parser::array_expression()
{
  type_t type = next_token->type;
  Type* exp_type = NULL;
  
  if (type == TOK_OPEN_BRACKET) {
    match(TOK_OPEN_BRACKET);
    try {
      exp_type = expression(false);
      if (exp_type && exp_type->type() != TYPE_INT)
        throw Error("Array index must be an integer\n");
      if (exp_type && !exp_type->is_symbol()) delete exp_type;
    }
    catch (Error& e) { std::cerr << e.what(); find(TOK_CLOSE_BRACKET); }
    match(TOK_CLOSE_BRACKET);
    return true;
  }
  return false;
}

void Parser::if_statement()
{
  Type* exp_type = NULL;
  
  match(RESERVED_IF);
  match(TOK_OPEN_PAREN);
  try {
    exp_type = expression(false);
    if (exp_type && exp_type->type() != TYPE_BOOL)
      throw Error("Must be a boolean expression in the if condition.\n");
  }
  catch (Error& e) { std::cerr << e.what(); find(TOK_CLOSE_PAREN); }
  if (exp_type && !exp_type->is_symbol()) {
    if(exp_type && !exp_type->is_symbol()) delete exp_type;
  }
  match(TOK_CLOSE_PAREN);
  match(RESERVED_THEN);
  
  int else_label = codegen->if_false("else");
  
  statements();
  follow_if(else_label);
}

void Parser::follow_if(int lab)
{
  type_t type = next_token->type;
  if (type == RESERVED_END) {
    match(RESERVED_END);
    match(RESERVED_IF);
    codegen->label("else", lab);
  }
  else if (type == RESERVED_ELSE) {
    match(RESERVED_ELSE);
    int after_else_label = codegen->next_label();

    codegen->goto_("next", after_else_label);
    codegen->label("else", lab);

    try { statements(); }
    catch (Error& e) { std::cerr << e.what(); find(RESERVED_END); }
    match(RESERVED_END);
    match(RESERVED_IF);
    codegen->label("next", after_else_label);
  }
  else { // error, not allowed
      throw SyntaxError("Missing end or else after if clause.", ZERO, ZERO);
  }
}

void Parser::loop_statement()
{
  Type* exp_type = NULL;
  
  match(RESERVED_FOR);
  match(TOK_OPEN_PAREN);
  
  int condition_label = codegen->next_label();
  codegen->goto_("loopcondition", condition_label);
  
  int assignment_label = codegen->next_label();
  codegen->label("loopassignment", assignment_label);
  
  try { assignment(); }
  catch (Error& e) { std::cerr << e.what(); find(TOK_SEMICOLON); }
  match(TOK_SEMICOLON);

  codegen->label("loopcondition", condition_label);

  try {
    exp_type = expression(false);
    if (exp_type->type() != TYPE_BOOL)
      throw Error("Must be a boolean expression in the for condition.\n");
  }
  catch (Error& e) { std::cerr << e.what(); find(TOK_CLOSE_PAREN); }
  if (exp_type && !exp_type->is_symbol()) delete exp_type;
  match(TOK_CLOSE_PAREN);

  int postloop_label = codegen->if_false("postloop");

  try { statements_(); }
  catch (Error& e) { std::cerr << e.what(); find(RESERVED_END); }
  match(RESERVED_END);
  match(RESERVED_FOR);

  codegen->goto_("loopassignment", assignment_label);
  codegen->label("postloop", postloop_label);
}

void Parser::return_statement()
{
  match(RESERVED_RETURN);
  codegen->callee_return(false);
}

std::string Parser::identifier()
{
  std::string id;
  if (next_token->type == TOK_IDENTIFIER) {
    id = next_token->string;
    match(TOK_IDENTIFIER);
  }
  return id;
}

Type* Parser::expression(bool out_param)
{
  type_t type = next_token->type;
  Type* expression_type = NULL;
  if (type == TOK_OPEN_PAREN || type == TOK_IDENTIFIER || type == TYPE_FLOAT || type == TYPE_INT
      || type == TOK_MINUS || type == TYPE_STRING || type == RESERVED_TRUE || type == RESERVED_FALSE)
  {
    expression_type = arithop(out_param);
    expression_(expression_type, out_param);
  }
  else if (type == RESERVED_NOT) {
    match(RESERVED_NOT);
    expression_type = arithop(false);
    expression_(expression_type, out_param);
    codegen->invert();
  }
  else {
    throw SyntaxError("Missing identifier, expression, boolean value, or constant value.", ZERO, ZERO);
  }
  return expression_type;
}

void Parser::expression_(Type*& lhs_type, bool out_param)
{
  Type* rhs_type = NULL;
  if (out_param && (next_token->type == TOK_AND) && (next_token->type == TOK_OR))
    bad_out_param = true;
  while (true) {
    type_t type = next_token->type;
    if (type == TOK_AND) { 
      match(TOK_AND);
      rhs_type = arithop(false);
      Type* t = arithmetic_typecheck(lhs_type, rhs_type, TOK_AND);
      codegen->arithmetic_operation(t->type(), "&");
      lhs_type = t;
      continue;
    }
    else if (type == TOK_OR) {
      match(TOK_OR);
      rhs_type = arithop(false);
      Type* t = arithmetic_typecheck(lhs_type, rhs_type, TOK_OR);
      codegen->arithmetic_operation(t->type(), "|");
      lhs_type = t;
      continue;
    }
    else
      break;	// e production
  }
}

Type* Parser::arithop(bool out_param)
{
  type_t type = next_token->type;
  Type* arithop_type = NULL;
  if (type == TOK_OPEN_PAREN || type == TOK_IDENTIFIER || type == TYPE_FLOAT || type == TYPE_INT
    || type == TOK_MINUS || type == TYPE_STRING || type == RESERVED_TRUE || type == RESERVED_FALSE)
  {
    arithop_type = relation(out_param);
    arithop_(arithop_type, out_param);
  }
  else {
    throw SyntaxError("Missing identifier, expression, boolean value, or constant value.", ZERO, ZERO);
  }
  return arithop_type;
}

void Parser::arithop_(Type*& lhs_type, bool out_param)
{
  Type* rhs_type = NULL;
  if (out_param && (next_token->type == TOK_PLUS) && (next_token->type == TOK_MINUS))
    bad_out_param = true;
  while (true) {
    type_t type = next_token->type;
    if (type == TOK_PLUS) {
      match(TOK_PLUS);
      rhs_type = relation(false);
      Type* t = arithmetic_typecheck(lhs_type, rhs_type, TOK_PLUS);
      codegen->arithmetic_operation(t->type(), "+");
      lhs_type = t;
      continue;
    }
    else if (type == TOK_MINUS) {
      match(TOK_MINUS);
      rhs_type = relation(false);
      Type* t = arithmetic_typecheck(lhs_type, rhs_type, TOK_MINUS);
      codegen->arithmetic_operation(t->type(), "-");
      lhs_type = t;
      continue;
    }
    else
      break;	// e production
  }
}

Type* Parser::relation(bool out_param)
{
  type_t type = next_token->type;
  Type* relation_type = NULL;
  if (type == TOK_OPEN_PAREN || type == TOK_IDENTIFIER || type == TYPE_FLOAT || type == TYPE_INT
	      || type == TOK_MINUS || type == TYPE_STRING || type == RESERVED_TRUE || type == RESERVED_FALSE)
  {
    relation_type = term(out_param);
    relation_(relation_type, out_param);
  }
  else {
    throw SyntaxError("Missing identifier, expression, boolean value, or constant value.", ZERO, ZERO);
  }
  return relation_type;
}

void Parser::relation_(Type*& lhs_type, bool out_param)
{
  Type* rhs_type = NULL;
  if (out_param && (next_token->type == TOK_RELOP))
    bad_out_param = true;
  while (true) {
    type_t type = next_token->type;
    if (type == TOK_RELOP) {
      relative_op_t relop = next_token->value.relop;
      match(TOK_RELOP);
      rhs_type = term(false);
      
      type_t left_type = lhs_type->type();
      type_t right_type = rhs_type->type();
      if ((left_type == TYPE_INT && right_type == TYPE_BOOL) || (left_type == TYPE_BOOL && right_type == TYPE_INT)) {
        if (left_type == TYPE_INT)
          codegen->valid_data_check(false);
        else
          codegen->valid_data_check(true);
      }
      else if (*lhs_type != *rhs_type)
        throw InvalidExpression(lhs_type, rhs_type, TOK_RELOP);
      
      // Must also be a valid type
      if (left_type == TYPE_STRING)
        throw InvalidOp(TOK_RELOP, left_type);
      
      // Result is always type boolean
      if (lhs_type && !lhs_type->is_symbol()) delete lhs_type;
      if (rhs_type && !rhs_type->is_symbol()) delete rhs_type;
      lhs_type = new Type(TYPE_BOOL, false, 1);
      
      codegen->relation_operation(relop);
      continue;
    }
    else
      break; // e production
  }
}

Type* Parser::term(bool out_param)
{
  type_t type = next_token->type;
  Type* term_type = NULL;
  if (type == TOK_OPEN_PAREN || type == TOK_IDENTIFIER || type == TYPE_FLOAT || type == TYPE_INT
      || type == TOK_MINUS || type == TYPE_STRING || type == RESERVED_TRUE || type == RESERVED_FALSE)
  {
    term_type = factor(out_param);
    term_(term_type, out_param);
  }
  else {
    throw SyntaxError("Missing identifier, expression, boolean value, or constant value.", ZERO, ZERO);
  }
  return term_type;
}

void Parser::term_(Type*& lhs_type, bool out_param)
{
  Type* rhs_type = NULL;
  if (out_param && (next_token->type == TOK_MULTIPLY) && (next_token->type == TOK_DIVIDE))
    bad_out_param = true;
  while (true) {
    type_t type = next_token->type;
    if (type == TOK_MULTIPLY) {
      match(TOK_MULTIPLY);
      rhs_type = factor(false);
      Type* t = arithmetic_typecheck(lhs_type, rhs_type, TOK_MULTIPLY);
      codegen->arithmetic_operation(t->type() , "*");
      lhs_type = t;
      continue;
    }
    else if (type == TOK_DIVIDE) {
      match(TOK_DIVIDE);
      rhs_type = factor(false);
      Type* t = arithmetic_typecheck(lhs_type, rhs_type, TOK_DIVIDE);
      codegen->arithmetic_operation(t->type(), "/");
      lhs_type = t;
      continue;
    }
    else
      break; // e production
  }
}

Type* Parser::factor(bool out_param)
{
  type_t type = next_token->type;
  Type* factortype = NULL;
  
  if (out_param && (type != TOK_IDENTIFIER))
      bad_out_param = true;
  
  if (type == TOK_OPEN_PAREN) {
    match(TOK_OPEN_PAREN);
    try { factortype = expression(false); }
    catch (Error& e) { std::cerr << e.what(); find(TOK_CLOSE_PAREN); }
    match(TOK_CLOSE_PAREN);
  }
  else if (type == TOK_IDENTIFIER || type == TYPE_FLOAT || type == TYPE_INT) {
    factortype = factor_(out_param);
  }
  else if (type == TOK_MINUS) {
    match(TOK_MINUS);
    factortype = factor_(false);
    if (factortype->type() == TYPE_STRING || factortype->type() == TYPE_BOOL || factortype->array())
      throw InvalidOp(TOK_MINUS, factortype->type());
    codegen->change_sign(factortype->type());
  }
  else if (type == TYPE_STRING) {
    factortype = string();
  }
  else if (type == RESERVED_FALSE) {
    match(RESERVED_FALSE);
    codegen->get_bool_value(false);
    factortype = new Type(TYPE_BOOL, false, 1);
  }
  else if (type == RESERVED_TRUE) {
    match(RESERVED_TRUE);
    codegen->get_bool_value(true);
    factortype = new Type(TYPE_BOOL, false, 1);
  }
  else {
    throw SyntaxError("Missing identifier, expression, boolean value, or constant value.", ZERO, ZERO);
  }
  return factortype;
}

Type* Parser::factor_(bool out_param)
{
  type_t type = next_token->type;
  Type* factor_type = NULL;
  if (type == TOK_IDENTIFIER) {
    factor_type = name(out_param);
  }
  else if (type == TYPE_FLOAT || type == TYPE_INT) {
    if (out_param)
      bad_out_param = true;
    factor_type = number(false);
  }
  else {
    throw SyntaxError("Missing identifier, expression, boolean value, or constant value.", ZERO, ZERO);
  }
  return factor_type;
}

Type* Parser::name(bool out_param)
{
  std::string id = identifier();
  bool arr_exp_type = array_expression();
  
  // Is an array and has brackets
  Symbol* id_sym = get_symbol(id);
  id_sym->used = true;
  if (out_param) id_sym->initialized = true;
  if (id_sym->symbol_type.array() && arr_exp_type) {

    if (out_param)
      codegen->calculate_array_element_address(id_sym, true, false, true);

    codegen->calculate_array_element_address(id_sym, true, out_param, false);

    if (id_sym->symbol_type.type() != TYPE_FLOAT)
      codegen->get_indexed_value(true);
    else
      codegen->get_indexed_value(false);

    Type* type = new Type(id_sym->symbol_type.type(), false, 1);
    return type;
  }
  // Is an array, but no brackets
  else if (id_sym->symbol_type.array()) {
    if (out_param)
      codegen->calculate_address(id_sym);
    codegen->calculate_address(id_sym);
    return &id_sym->symbol_type;
  }
  // Has brackets but not an array
  else if (arr_exp_type) {
    throw Error("Not an array type\n");
  }
  // Not an array, and doesnt have brackets
  else {
    
    if (out_param)
      codegen->calculate_address(id_sym);
    
    if (id_sym->symbol_type.type() != TYPE_FLOAT)
      codegen->get_value(true, id_sym);
    else
      codegen->get_value(false, id_sym);

    return &id_sym->symbol_type;
  }
}

void Parser::argument_list(Symbol* procedure)
{
  type_t type = next_token->type;
  int num_out_params = 0;
  std::stack<Symbol*> args;
  if (type == TOK_OPEN_PAREN || type == TOK_IDENTIFIER || type == TYPE_FLOAT || type == TYPE_INT
      || type == TOK_MINUS || type == TYPE_STRING || type == RESERVED_TRUE || type == RESERVED_FALSE)
  {
    Type* exp_type = NULL;
    
    // Check the type for the first argument in procedure call
    if (procedure->params == NULL) {
      throw Error("Too many arguments in procedure call\n");
    }
    
    if (procedure->params->direction == DIRECTION_OUT)
      exp_type = expression(true);
    else
      exp_type = expression(false);
    
    if (bad_out_param) {
      std::string integer = std::to_string(1);
      bad_out_param = false;
      throw Error("A modifiable l-value is expected in argument "+integer+"\n");
    }
    
    try { type_match(&procedure->params->symbol_type, exp_type, 1); }
    catch (InvalidAssignment& e) { 
      std::cerr << e.what();
      if (procedure->params->next)
        find(TOK_COMMA);
      else
        find(TOK_CLOSE_PAREN);
    }
    if (exp_type && !exp_type->is_symbol()) delete exp_type;
    
    // Push argument into a stack so they can be processed in reverse order
    args.push(procedure->params);
    codegen->spill_register();
    if (procedure->params->direction == DIRECTION_OUT) {
      num_out_params++;
      codegen->spill_register();
    }
    
    // last_arg should be null or we are missing arguments
    Symbol* last_arg = argument_list_(procedure->params->next, args, num_out_params);
    if(last_arg != NULL)
      throw Error("Not enough arguments in procedure call\n");
    
    codegen->push_parameters(args, num_out_params);
  }
  else {
    if (procedure->params != NULL)
      throw Error("Not enough arguments in procedure call\n");
  }
}

Symbol* Parser::argument_list_(Symbol* next_param, std::stack<Symbol*> &args, int &num_out_params)
{
  Symbol* current_param = next_param;
  int argnum = 2;
  while (true) {
    type_t type = next_token->type;
    if (type == TOK_COMMA) {
      match(TOK_COMMA);
      Type* exp_type = NULL;
      // Check types in procedure call
      if (current_param == NULL) {
        throw Error("Too many arguments in procedure call\n");
      }
      if (current_param->direction == DIRECTION_OUT)
        exp_type = expression(true);
      else
        exp_type = expression(false);
      
      if (bad_out_param) {
        std::string integer = std::to_string(argnum);
        bad_out_param = false;
        throw Error("A modifiable l-value is expected in argument "+integer+"\n");
      }
      
      // Check types
      try { type_match(&current_param->symbol_type, exp_type, argnum); }
      catch (InvalidAssignment& e) { 
      std::cerr << e.what();
      if (current_param->next)
        find(TOK_COMMA);
      else
        find(TOK_CLOSE_PAREN);
      }
      if (exp_type && !exp_type->is_symbol()) delete exp_type;

      // Push argument onto stack
      args.push(current_param);
      codegen->spill_register();
      if (current_param->direction == DIRECTION_OUT) {
        codegen->spill_register();
        num_out_params++;
      }
      current_param = current_param->next;
      argnum++;
      continue;
    }
    else
      break;	// e production
  }
  return current_param;
}

Type* Parser::number(bool arraysize)
{
  type_t type = next_token->type;
  if (type == TYPE_INT) {
    if (!arraysize) {
      codegen->get_constant(true, next_token);
    }
    match(TYPE_INT);
    Type* t = new Type(TYPE_INT, false, 1);
    return t;
  }
  else if (type == TYPE_FLOAT) {
    codegen->get_constant(false, next_token);
    match(TYPE_FLOAT);
    Type* t = new Type(TYPE_FLOAT, false, 1);
    return t;
  }
  else { // error, not allowed
    throw Error("A constant value is expected.\n");
  }
}

Type* Parser::string()
{
  if (next_token->type == TYPE_STRING) {
    codegen->get_string_literal(next_token);
    match(TYPE_STRING);
    Type* t = new Type(TYPE_STRING, false, 1);
    return t;
  }
  return 0;
}

void Parser::add_to_symbol_table(Symbol* sym)
{  
  if (sym->is_global) {
    if (outer_symbol_map == symbol_table.top() ) {
      map_iterator = global_symbol_map->find(sym->name);
      if (map_iterator != global_symbol_map->end())
        throw Redeclaration(sym->name);
      else {
        std::pair<std::string,Symbol*> pair(sym->name,sym);
      global_symbol_map->insert(pair);
      sym->ref_count++;
      }
    }
    else {
      Scanner::report_warning("Line "+std::to_string(Scanner::line_number)+": Ignoring the 'global' specifier. It should not be used in this scope\n");
      map_iterator = symbol_table.top()->find(sym->name);
      if (map_iterator != symbol_table.top()->end())
        throw Redeclaration(sym->name);
      else {
        std::pair<std::string,Symbol*> pair(sym->name,sym);
        symbol_table.top()->insert(pair);
        sym->ref_count++;
      }
    }
  }
  else {
    map_iterator = symbol_table.top()->find(sym->name);
    if (map_iterator != symbol_table.top()->end())
      throw Redeclaration(sym->name);
    else {
      std::pair<std::string,Symbol*> pair(sym->name,sym);
      symbol_table.top()->insert(pair);
      sym->ref_count++;
    }
  }
}

Symbol* Parser::get_symbol(std::string id)
{
  map_iterator = symbol_table.top()->find(id);
  // If the symbol is not in the current scope, check global symbol table
  if (map_iterator == symbol_table.top()->end()) {
    map_iterator = global_symbol_map->find(id);
    if (map_iterator == global_symbol_map->end()) {
      // Not in either symbol table
      throw NoDeclaration(id);
    }
  }
  return map_iterator->second;
}

void Parser::type_match(Type* lhs_type, Type* rhs_type, int argnum)
{
  // TODO only allow array assignment for params
  if (*lhs_type != *rhs_type)
    throw InvalidAssignment(lhs_type, rhs_type, argnum);
}

Type* Parser::arithmetic_typecheck(Type* lhs_type, Type* rhs_type, type_t op_type)
{
  type_t left_type = lhs_type->type();
  type_t right_type = rhs_type->type();
  Type* ret_type = NULL;
  
  // If conversion is necessary
  if ((left_type == TYPE_INT && right_type == TYPE_FLOAT) || (left_type == TYPE_FLOAT && right_type == TYPE_INT)) {
    if (left_type == TYPE_INT) {
      codegen->cast_to_float(false);
    }
    else {
      codegen->cast_to_float(true);
    }
    ret_type = new Type(TYPE_FLOAT, false, 1);
  }
  else if (*lhs_type != *rhs_type)
    throw InvalidExpression(lhs_type, rhs_type, op_type);
  else
    ret_type = new Type(left_type, false, 1);
  
  // Must also be a valid type
  if (lhs_type->array() || (left_type == TYPE_BOOL && (op_type != TOK_AND && op_type != TOK_OR)) || (left_type == TYPE_STRING))
    throw InvalidOp(op_type, left_type);
  
  if (rhs_type && !rhs_type->is_symbol()) delete rhs_type;
  if (lhs_type && !lhs_type->is_symbol()) delete lhs_type;
  return ret_type;
}
