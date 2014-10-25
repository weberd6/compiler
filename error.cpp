
#include <sstream>
#include "error.h"

int Scanner::num_errors;
int Scanner::num_warnings;

Error::Error() noexcept
{
  Scanner::num_errors++;
}

Error::Error(const std::string &message) noexcept : msg(message)
{
  Scanner::num_errors++;
}

NoDeclaration::NoDeclaration(std::string wrd) noexcept
{
  word = wrd;
}

Redeclaration::Redeclaration(std::string wrd) noexcept
{
  word = wrd;
}

InvalidAssignment::InvalidAssignment(Type* lhs_type, Type* rhs_type, int argnum) noexcept
{
  left_type = lhs_type;
  right_type = rhs_type;
  arg = argnum;
}

InvalidExpression::InvalidExpression(Type* lhs_type, Type* rhs_type, type_t optype) noexcept
{
  left_type = lhs_type;
  right_type = rhs_type;
  op_type = optype;
}

InvalidOp::InvalidOp(type_t type1, type_t type2) noexcept
{
  op_type = type1;
  type_type = type2;
}

const char * Error::what() const noexcept
{
  std::stringstream stream;
  stream << "ERROR: Line " << Scanner::line_number << ": ";
  stream << msg;
  return stream.str().c_str();
}

const char * SyntaxError::what() const noexcept
{
  std::stringstream stream;
  std::string strng;
  stream << "ERROR: Line " << Scanner::line_number << ": ";
  stream << msg;
  if (expected && found) {
    stream << "A ";
    stream << Scanner::print_token(expected, strng);
    stream << " was expected but a ";
    stream << Scanner::print_token(found, strng);
    stream << " was found.";
  }
  stream << std::endl;
  return stream.str().c_str();
}

const char * InvalidAssignment::what() const noexcept
{
  std::stringstream stream;
  std::string strng;
  stream << "ERROR: Line " << Scanner::line_number << ": ";
  stream << "Invalid assignment to type ";
  stream << Scanner::print_token(left_type->type(), strng);
  if (left_type->array()) { stream << "[" << left_type->arraysize() << "]"; }
  stream << " from type ";
  stream << Scanner::print_token(right_type->type(), strng);
  if (right_type->array()) { stream << "[" << right_type->arraysize() << "]"; }
  if (arg) { stream << " in argument " << arg; }
  stream << std::endl;
  
  return stream.str().c_str();
}

const char * InvalidExpression::what() const noexcept
{
  std::stringstream stream;
  std::string strng;
  stream << "ERROR: Line " << Scanner::line_number << ": "; // TODO more detail in error
  stream << "Invalid type compatability between ";
  stream << Scanner::print_token(left_type->type(), strng);
  stream << " and ";
  stream << Scanner::print_token(right_type->type(), strng);
  stream << " in expression near operator " << Scanner::print_token(op_type, strng);
  stream << std::endl;
  
  return stream.str().c_str();
}

const char * InvalidOp::what() const noexcept
{
  std::stringstream stream;
  std::string strng;
  stream << "ERROR: Line " << Scanner::line_number << ": ";
  stream << "Operation ";
  stream << Scanner::print_token(op_type, strng);
  stream << " not defined for type ";
  stream << Scanner::print_token(type_type, strng);
  stream << std::endl;
  
  return stream.str().c_str();
}

const char * NoDeclaration::what() const noexcept
{
  std::stringstream stream;
  stream << "ERROR: Line " << Scanner::line_number << ": ";
  stream << "'" << word << "'" << " has not been declared in the current scope\n";

  return stream.str().c_str();
}

const char * Redeclaration::what() const noexcept
{
  std::stringstream stream;
  stream << "ERROR: Line " << Scanner::line_number << ": ";
  stream << "'" << word << "'" << " has already been declared in the current scope.\n";
  
  return stream.str().c_str();
}