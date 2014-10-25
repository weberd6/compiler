#include "codegenerator.h"
#include "parser.h"

Code_generator::Code_generator(std::string filename)
{
  reg = 0;
  static_address = 0;
  label_num = 0;
  
  // Strip off extension of input file and append .c for output file
  int lastindex = filename.find_last_of("."); 
  rawname = filename.substr(0, lastindex); 
  output_file.open(rawname+".c", std::ios_base::out | std::ios_base::trunc);
}

Code_generator::~Code_generator()
{
  output_file.close();
  std::string command = "gcc -g -o "+rawname+" "+rawname+".c";
  if (!Scanner::num_errors)
    system(command.c_str()); // Compile C to native code
}

int Code_generator::next_label()
{
  return ++label_num;
}

void Code_generator::main_runtime()
{
  output_file << "#include \"runtime.c\"\n\n";
  output_file << "int main() {\n";
  output_file << "\tgoto start;\n";

  Symbol* sym;

  sym = new Symbol("getbool", true);
  sym->params = new Symbol("bb", false, TYPE_BOOL, 1);
  sym->params->address = 3;
  sym->params->direction = DIRECTION_OUT;
  Parser::add_to_symbol_table(sym);
  output_file << "getbool:\n";
  output_file << "\tMM[Reg[FP]+3] = getBool();\n";
  callee_return(true);

  sym = new Symbol("getinteger", true);
  sym->params = new Symbol("ii", false, TYPE_INT, 1);
  sym->params->address = 3;
  sym->params->direction = DIRECTION_OUT;
  Parser::add_to_symbol_table(sym);
  output_file << "getinteger:\n";
  output_file << "\tMM[Reg[FP]+3] = getInteger();\n";
  callee_return(true);

  sym = new Symbol("getfloat", true);
  sym->params = new Symbol("ff", false, TYPE_FLOAT, 1);
  sym->params->address = 3;
  sym->params->direction = DIRECTION_OUT;
  Parser::add_to_symbol_table(sym);
  output_file << "getfloat:\n";
  output_file << "\t*((double*)&MM[Reg[FP]+3]) = getFloat();\n";
  callee_return(true);

  sym = new Symbol("getstring", true);
  sym->params = new Symbol("ss", false, TYPE_STRING, 1);
  sym->params->address = 3;
  sym->params->direction = DIRECTION_OUT;
  Parser::add_to_symbol_table(sym);
  output_file << "getstring:\n";
  output_file << "\tMM[Reg[FP]+3] = (long long)getString();\n";
  callee_return(true);

  sym = new Symbol("putbool", true);
  sym->params = new Symbol("b", false, TYPE_BOOL, 1);
  sym->params->address = 2;
  sym->params->direction = DIRECTION_IN;
  Parser::add_to_symbol_table(sym);
  output_file << "putbool:\n";
  output_file << "\tputBool(MM[Reg[FP]+2]);\n";
  callee_return(true);

  sym = new Symbol("putinteger", true);
  sym->params = new Symbol("i", false, TYPE_INT, 1);
  sym->params->address = 2;
  sym->params->direction = DIRECTION_IN;
  Parser::add_to_symbol_table(sym);
  output_file << "putinteger:\n";
  output_file << "\tputInteger(MM[Reg[FP]+2]);\n";
  callee_return(true);

  sym = new Symbol("putfloat", true);
  sym->params = new Symbol("f", false, TYPE_FLOAT, 1);
  sym->params->address = 2;
  sym->params->direction = DIRECTION_IN;
  Parser::add_to_symbol_table(sym);
  output_file << "putfloat:\n";
  output_file << "\tputFloat(*((double*)&MM[Reg[FP]+2]));\n";
  callee_return(true);

  sym = new Symbol("putstring", true);
  sym->params = new Symbol("s", false, TYPE_STRING, 1);
  sym->params->address = 2;
  sym->params->direction = DIRECTION_IN;
  Parser::add_to_symbol_table(sym);
  output_file << "putstring:\n";
  output_file << "\tReg[0] = MM[Reg[FP]+2];\n";
  output_file << "\tputString((char*)Reg[0]);\n";
  callee_return(true);
}

void Code_generator::start()
{
  output_file << "start:\n";
  output_file << "\tinit(" << static_address << ");\n";
}

void Code_generator::exit()
{
  output_file << "\treturn 0;\n}\n";
}

void Code_generator::enter_procedure()
{
  std::stringstream* sstream = new std::stringstream;
  procedure_code.push(sstream);
}

void Code_generator::emit_procedure()
{
  if (procedure_code.top()->rdbuf()->in_avail()) // Needed to add this in case of empty buffer which screws up output_file
    output_file << procedure_code.top()->rdbuf();
  if (!procedure_code.empty())
    delete procedure_code.top();
  procedure_code.pop();
}

void Code_generator::arithmetic_operation(type_t type, const char* op)
{
  int reg1 = reg_num_stack.top() - 1;
  if (type != TYPE_FLOAT)
    *procedure_code.top() << "\tReg[" << reg1 << "] = " << "Reg[" << reg1 << "] " << op << " Reg[" << reg_num_stack.top() << "];\t// expression = operand1 & operand2\n";
  else
    *procedure_code.top() << "\t*((double*)&Reg[" << reg1 << "] = " << "*((double*)&Reg[" << reg1 << "]) " << op << " *((double*)&Reg[" << reg_num_stack.top() << "]);\t// expression = operand1 & operand2\n";
  reg_num_stack.pop();
  reg--;
}

void Code_generator::relation_operation(relative_op_t relop)
{
  int reg1 = reg_num_stack.top() - 1;

  *procedure_code.top() << "\tReg[" << reg1 << "] = " << "Reg[" << reg1 << "] ";
  output_relop(relop);
  *procedure_code.top() << " Reg[" << reg_num_stack.top() << "];\t// relation = operand1 relop operand2\n";

  reg_num_stack.pop();
  reg--;
}

void Code_generator::invert()
{
  *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = ~Reg[" << reg_num_stack.top() << "];\t\t//\n";
}

void Code_generator::change_sign(type_t type)
{
  if (type == TYPE_INT)
    *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = - Reg[" << reg_num_stack.top() << "];\t\t//\n";
  else
    *procedure_code.top() << "\t*((double*)Reg[" << reg_num_stack.top() << "]) = - *((double*)Reg[" << reg_num_stack.top() << "]);\t\t//\n";
}

void Code_generator::calculate_address(Symbol *sym)
{
  reg_num_stack.push(reg++);
  if (sym->is_global)
    *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = " << sym->address <<";\t\t// Get address of array\n";
  else if (sym->id_type == ID_VARIABLE)
    *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = Reg[FP] - " << sym->address <<";\t\t// Get address of array\n";
  else
    *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = Reg[FP] + " << sym->address <<";\t\t// Get address of array\n";
}

void Code_generator::calculate_array_element_address(Symbol* sym, bool top, bool addr_already, bool copy_index)
{

  if (copy_index) {
    int reg1 = reg++;
    *procedure_code.top() << "\tReg[" << reg1 << "] = Reg[" << reg_num_stack.top() << "];\n";
  }
    
  if (addr_already) {
    reg_num_stack.push(reg-1);
  }
  
  if (top) {
    if (sym->is_global)
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = " << sym->address <<" + Reg[" << reg_num_stack.top() << "];\t\t// Address of " << sym->name << " + offset\n";
    else if (sym->id_type == ID_VARIABLE) {
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = -" << sym->address <<" + Reg[" << reg_num_stack.top() << "];\t\t// Address of " << sym->name << " + offset\n";
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = Reg[FP] + Reg[" << reg_num_stack.top() << "];\n";
    }
    else {
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = " << sym->address <<" + Reg[" << reg_num_stack.top() << "];\t\t// Address of " << sym->name << " + offset\n";
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = Reg[FP] + Reg[" << reg_num_stack.top() << "];\n";
    }
  }
  else {
    int reg1 = reg_num_stack.top()-1;
    if (sym->is_global)
      *procedure_code.top() << "\tReg[" << reg1 << "] = " << sym->address <<" + Reg[" << reg1 << "];\t\t// Address of " << sym->name << " + offset\n";
    else if (sym->id_type == ID_VARIABLE) {
      *procedure_code.top() << "\tReg[" << reg1 << "] = -" << sym->address <<" + Reg[" << reg1 << "];\t\t// Address of " << sym->name << " + offset\n";
      *procedure_code.top() << "\tReg[" << reg1 << "] = Reg[FP] + Reg[" << reg1 << "];\n";
    }
    else {
      *procedure_code.top() << "\tReg[" << reg1 << "] = " << sym->address <<" + Reg[" << reg1 << "];\t\t// Address of " << sym->name << " + offset\n";
      *procedure_code.top() << "\tReg[" << reg1 << "] = Reg[FP] + Reg[" << reg1 << "];\n";
    }
  }
} 

void Code_generator::cast_to_float(bool top)
{
  int reg1 = reg_num_stack.top()-1;
  if (top)
    *procedure_code.top() << "\t*((double*)&Reg[" << reg_num_stack.top() << "]) = (double)Reg[" << reg_num_stack.top() << "];// Conversion\n";
  else
    *procedure_code.top() << "\t*((double*)&Reg[" << reg1 << "]) = (double)Reg[" << reg1 << "]);// Conversion\n";
}

int Code_generator::if_false(std::string labelname)
{
  int else_label = ++label_num;
  *procedure_code.top() << "\tif (Reg[" << reg_num_stack.top() << "] == false) goto " << labelname << else_label << ";\n";
  reg_num_stack.pop();
  reg--;
  return else_label;
}

int Code_generator::if_true(std::string labelname)
{
  int else_label = ++label_num;
  *procedure_code.top() << "\tif (Reg[" << reg_num_stack.top() << "] == true) goto " << labelname << else_label << ";\n";
  reg_num_stack.pop();
  reg--;
  return else_label;
}

void Code_generator::label(std::string labelname, int num)
{
  if (num == -1)
    *procedure_code.top() << labelname << ":\n";
  else
    *procedure_code.top() << labelname << num << ":\n";
}

void Code_generator::goto_(std::string labelname, int num)
{
  if (num == -1)
    *procedure_code.top() << "\tgoto " << labelname << ";\n";
  else
    *procedure_code.top() << "\tgoto " << labelname << num << ";\n";
}

void Code_generator::stack_alloc_param(Symbol* sym, bool in_param)
{
  if (in_param) {
    sym->address = 2 + rel_fp_address; // Add size of frame pointer and return address to offset
    rel_fp_address += sym->width;
  }
  else {
    sym->address = 3 + rel_fp_address;
    rel_fp_address += (sym->width + 1);
  }
}

void Code_generator::stack_alloc_local(Symbol* sym)
{
  *procedure_code.top() << "\tReg[SP] = Reg[SP] - " << sym->width << ";\t// Allocate space on stack for local variable\n";
  rel_fp_address += sym->width;
  sym->address = rel_fp_address;
}

void Code_generator::alloc_static(Symbol* sym)
{
  sym->address = static_address;
  static_address = static_address + sym->width;
}

void Code_generator::reset_fp_address()
{
  rel_fp_address = 0;
}

void Code_generator::push_parameters(std::stack<Symbol*> &args, int num_out_params)
{
  Symbol *sym;
  int size = args.size();
  int i = size + num_out_params - 1;
  while (!args.empty()) { // Push args on to stack in reverse order
    sym = args.top();
    
    *procedure_code.top() << "\tReg[SP] = Reg[SP] - " << sym->width << ";\t\t// Make space on stack for argument\n";
    if (sym->symbol_type.array()) {
      *procedure_code.top() << "\tmemcpy(&MM[Reg[SP]], &MM[Reg[" << i << "]], 8*" << sym->width << ");\t\t// Copy array\n";
    }
    else
      *procedure_code.top() << "\tMM[Reg[SP]] = Reg[" << i << "];\t\t// Move argument to stack\n"; 
    reg--;
    i--;
    
    if (sym->direction == DIRECTION_OUT) {
      *procedure_code.top() << "\tReg[SP] = Reg[SP] - 1;\t\t// Allocate space for out param address\n";
      *procedure_code.top() << "\tMM[Reg[SP]] = Reg[" << i << "];\t\t// Push address for out parameter onto stack\n";
      i--;
      reg--;
    }
    
  args.pop();
  }
}

void Code_generator::call_procedure(std::string name)
{
  *procedure_code.top() << "\tReg[SP] = Reg[SP] - 1;\t\t// Allocate space for return address\n";
  *procedure_code.top() << "\tReg[" << reg << "] = (long long)&&post" << name << ++label_num << ";\n";
  *procedure_code.top() << "\tMM[Reg[SP]] = Reg[" << reg << "];\t\t// save return address\n";
  *procedure_code.top() << "\tReg[SP] = Reg[SP] - 1;\t\t// Allocate space for FP\n";
  *procedure_code.top() << "\tMM[Reg[SP]] = Reg[FP];\t\t// Save frame pointer\n";
  *procedure_code.top() << "\tReg[FP] = Reg[SP];\t\t// Move frame pointer to new position\n";
  *procedure_code.top() << "\tgoto " << name << ";\t\t// jump to procedure\n";
  *procedure_code.top() << "post" << name << label_num << ":\n";
}

void Code_generator::caller_return(Symbol* sym)
{
  while (sym != NULL) {
    if (sym->direction == DIRECTION_OUT) {

      *procedure_code.top() << "\tReg[" << reg << "] = MM[Reg[SP]];\t\t// Get address of out param off stack\n";
      *procedure_code.top() << "\tReg[SP] = Reg[SP] + 1;\n";
        
      if (sym->symbol_type.array())
        *procedure_code.top() << "\tmemcpy(&MM[Reg[" << reg << "]], &MM[Reg[SP]], 8*" << sym->width << "); // Get out param off stack\n";
      else {
        if (sym->symbol_type.type() != TYPE_FLOAT)
          *procedure_code.top() << "\tMM[Reg[" << reg << "]] = MM[Reg[SP]];\t\t// Get out param off stack\n";
        else
          *procedure_code.top() << "\t*((double*)&MM[Reg[" << reg << "]]) = *((double*)&MM[Reg[SP]]);\t// Get out param off stack\n";
      }
        
      *procedure_code.top() << "\tReg[SP] = Reg[SP] + " << sym->width << ";\t\t// Free output parameter\n";
    }
    else {
      *procedure_code.top() << "\tReg[SP] = Reg[SP] + " << sym->width << ";\t\t// Free input parameter\n";
    }
    sym = sym->next;
  }
}

void Code_generator::callee_return(bool runtime)
{
  if (!runtime) {
    *procedure_code.top() << "\tReg[SP] = Reg[FP];\t\t// Free local variables\n";
    *procedure_code.top() << "\tReg[FP] = MM[Reg[FP]];\t\t// Restore Frame pointer\n";
    *procedure_code.top() << "\tReg[SP] = Reg[SP] + 1;\t\t// Free space for FP\n";
    *procedure_code.top() << "\tReg[0] = MM[Reg[SP]];\t\t// Pop return address off the stack\n";
    *procedure_code.top() << "\tReg[SP] = Reg[SP] + 1;\t\t// Free return address space\n";
    *procedure_code.top() << "\tgoto *(void*)Reg[0];\t\t// Return\n";
  }
  else {
    output_file << "\tReg[SP] = Reg[FP];\t\t// Free local variables\n";
    output_file << "\tReg[FP] = MM[Reg[FP]];\t\t// Restore Frame pointer\n";
    output_file << "\tReg[SP] = Reg[SP] + 1;\t\t// Free space for FP\n";
    output_file << "\tReg[0] = MM[Reg[SP]];\t\t// Pop return address off the stack\n";
    output_file << "\tReg[SP] = Reg[SP] + 1;\t\t// Free return address space\n";
    output_file << "\tgoto *(void*)Reg[0];\t\t// Return\n";
  }
}

void Code_generator::get_bool_value(bool b)
{
  reg_num_stack.push(reg++);
  if (b)
    *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = true;\t\t// operand = true\n";
  else
    *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = false;\t\t// operand = false\n";
}

void Code_generator::get_string_literal(Token* next_token)
{
  reg_num_stack.push(reg++);
  *procedure_code.top() << "\tstrcpy((char*)&MM[" << static_address << "], " << "\"" << next_token->string.c_str() << "\"" << ");\t\t// operand = string\n";
  *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = (long long)&MM[" << static_address << "];\n";
  static_address = static_address + next_token->string.length()+1;
}

void Code_generator::get_constant(bool is_int, Token* next_token)
{
  reg_num_stack.push(reg++);
  if (is_int)
    *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = " << next_token->value.int_value << ";\t\t// operand = integer\n";
  else
    *procedure_code.top() << "\t*((double*)&Reg[" << reg_num_stack.top() << "]) = " << next_token->value.float_value << ";\t\t// operand = double\n";
}

void Code_generator::get_value(bool is_int, Symbol* sym)
{
  reg_num_stack.push(reg++);
  if (is_int) {
    if (sym->is_global)
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = MM[" << sym->address << "];\t\t// operand = static variable\n";
    else if (sym->id_type == ID_VARIABLE)
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = MM[Reg[FP] - " << sym->address << "];\t\t// operand = local variable\n";
    else
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = MM[Reg[FP] + " << sym->address << "];\t\t// operand = parameter\n";
  }
  else {
    if (sym->is_global)
      *procedure_code.top() << "\t*((double*)&Reg[" << reg_num_stack.top() << "]) = *((double*)&MM[" << sym->address << "]);\t\t// operand = static variable\n";
    else if (sym->id_type == ID_VARIABLE)
      *procedure_code.top() << "\t*((double*)&Reg[" << reg_num_stack.top() << "]) = *((double*)&MM[Reg[FP] - " << sym->address << "]);\t\t// operand = local variable\n";
    else
      *procedure_code.top() << "\t*((double*)&Reg[" << reg_num_stack.top() << "]) = *((double*)&MM[Reg[FP] + " << sym->address << "]);\t\t// operand = parameter\n";
  }
}

void Code_generator::get_indexed_value(bool is_int)
{
    if (is_int)
      *procedure_code.top() << "\tReg[" << reg_num_stack.top() << "] = MM[Reg[" << reg_num_stack.top() << "]];\t\t// Get array operand\n";
    else
      *procedure_code.top() << "\t*((double*)&Reg[" << reg_num_stack.top() << "]) = *((double*)&MM[Reg[" << reg_num_stack.top() << "]]);\t\t// Get array operand\n";
}

void Code_generator::assignment(Symbol *left, bool is_int)
{
  if (is_int) {
    if (left->is_global)
      *procedure_code.top() << "\tMM[" << left->address << "] = Reg[" << reg_num_stack.top() << "];\t\t// assign static\n";
    else if (left->id_type == ID_VARIABLE)
      *procedure_code.top() << "\tMM[Reg[FP] - " << left->address << "] = Reg[" << reg_num_stack.top() << "];\t\t// assign local variable\n";
    else
      *procedure_code.top() << "\tMM[Reg[FP] + " << left->address << "] = Reg[" << reg_num_stack.top() << "];\t\t// assign parameter\n";
  }
  else {
    if (left->is_global)
      *procedure_code.top() << "\tmemcpy(&MM[" << left->address << "], &Reg[" << reg_num_stack.top() << "], sizeof(double));\t\t// assign static\n";
    else if (left->id_type == ID_VARIABLE)
      *procedure_code.top() << "\tmemcpy(&MM[Reg[FP] - " << left->address << "], &Reg[" << reg_num_stack.top() << "], sizeof(double));\t\t// assign local variable\n";
    else
      *procedure_code.top() << "\tmemcpy(&MM[Reg[FP] + " << left->address << "], &Reg[" << reg_num_stack.top() << "], sizeof(double));\t\t// assign parameter\n";
  }
  reg_num_stack.pop();
  reg--;
}

void Code_generator::assign_indexed(Symbol *left, bool is_int)
{
  int reg1 = reg_num_stack.top()-1;
  if (is_int)
    *procedure_code.top() << "\tMM[Reg[" << reg1 << "]] = Reg[" << reg_num_stack.top() << "];\t\t// " << " + offset = expression\n";
  else
    *procedure_code.top() << "\tmemcpy(&MM[Reg[" << reg1 << "]], &Reg[" << reg_num_stack.top() << "], sizeof(double));\t\t// " << " + offset = expression\n";
  
  reg_num_stack.pop();
  reg--;
  reg--;
}

void Code_generator::spill_register()
{
  reg_num_stack.pop();
}

void Code_generator::valid_data_check(bool top)
{
  int reg1 = reg_num_stack.top()-1;
  if (top)
    *procedure_code.top() << "\tdataConversionCheck(Reg[" << reg_num_stack.top() << "]);\n";
  else
    *procedure_code.top() << "\tdataConversionCheck(Reg[" << reg1 << "]);\n";
}

void Code_generator::output_relop(relative_op_t relop) 
{
    switch (relop) {
        case IS_EQUAL:
            *procedure_code.top() << "==";
            break;
        case NOT_EQUAL:
            *procedure_code.top() << "!=";
            break;
        case GREATER_THAN:
            *procedure_code.top() << ">";
            break;
        case LESS_THAN:
            *procedure_code.top() << "<";
            break;
        case GREATER_OR_EQUAL:
            *procedure_code.top() << ">=";
            break;
        case LESS_OR_EQUAL:
            *procedure_code.top() << "<=";
            break;
        default:
            break;
    }
}