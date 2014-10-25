#ifndef ERROR_H
#define ERROR_H

#include <exception>
#include <string>
#include "scanner.h"
#include "symbol.h"

class Error : public std::exception {
public:
  Error() noexcept;
  Error(const std::string &message) noexcept;
  ~Error() noexcept {}
  virtual const char * what() const noexcept;
protected:
  std::string msg;
};

class SyntaxError : public Error {
public:
  SyntaxError(const std::string &message, type_t exp, type_t fnd) noexcept : Error(message), expected(exp), found(fnd) {}
  ~SyntaxError() noexcept {}
  virtual const char * what() const noexcept;
public:
  type_t expected;
  type_t found;
};

class InvalidAssignment : public Error {
public:
  InvalidAssignment(Type* lhs_type, Type* rhs_type, int argum) noexcept;
  ~InvalidAssignment() noexcept {}
  virtual const char * what() const noexcept;
private:
  Type* left_type;
  Type* right_type;
  int arg;
};

class InvalidExpression : public Error {
public:
  InvalidExpression(Type* lhs_type, Type* rhs_type, type_t optype) noexcept;
  ~InvalidExpression() noexcept {}
  virtual const char * what() const noexcept;
private:
  Type* left_type;
  Type* right_type;
  type_t op_type;
};

class InvalidOp : public Error {
public:
  InvalidOp(type_t type1, type_t type2) noexcept;
  ~InvalidOp() noexcept {}
  virtual const char * what() const noexcept;
private:
  type_t op_type;
  type_t type_type;
};

class NoDeclaration : public Error {
public:
  NoDeclaration(std::string wrd) noexcept;
  ~NoDeclaration() noexcept {}
  virtual const char * what() const noexcept;
private:
  std::string word;
};

class Redeclaration : public Error {
public:
  Redeclaration(std::string wrd) noexcept;
  ~Redeclaration() noexcept {}
  virtual const char * what() const noexcept;
private:
  std::string word;
};

#endif