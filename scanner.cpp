#include <cstdlib>
#include <iostream>
#include <sstream>

#include "scanner.h"

std::ifstream Scanner::input_file;
int Scanner::line_number;

Scanner::Scanner(std::string filename)
{
  init(filename);
  line_number = 1;
  num_errors = 0;
  num_warnings = 0;
}

Scanner::~Scanner()
{
  reserved_words.clear();
  input_file.close();
}

bool Scanner::init(std::string filename)
{
  file_name = filename;
  
  input_file.open(file_name, std::ios_base::in);
 
  // Setup reserved words table
  reserve(RESERVED_STRING, "string");
  reserve(RESERVED_INT, "integer");
  reserve(RESERVED_BOOL, "bool");
  reserve(RESERVED_FLOAT, "float");
  reserve(RESERVED_GLOBAL, "global");
  reserve(RESERVED_IN, "in");
  reserve(RESERVED_OUT, "out");
  reserve(RESERVED_IF, "if");
  reserve(RESERVED_THEN, "then");
  reserve(RESERVED_ELSE, "else");
  reserve(RESERVED_CASE, "case");
  reserve(RESERVED_FOR, "for");
  reserve(RESERVED_AND, "and");
  reserve(RESERVED_OR, "or" );
  reserve(RESERVED_NOT, "not");
  reserve(RESERVED_PROGRAM, "program");
  reserve(RESERVED_PROCEDURE, "procedure");
  reserve(RESERVED_BEGIN, "begin");
  reserve(RESERVED_RETURN, "return");
  reserve(RESERVED_END, "end");
  reserve(RESERVED_IS, "is");
  reserve(RESERVED_FALSE, "false");
  reserve(RESERVED_TRUE, "true");
  
  return true;
}

void Scanner::reserve(type_t type, std::string word)
{
  reserved_words[word] = type;  
}

bool Scanner::is_reserved(std::string word)
{
  reserved_words_iterator = reserved_words.find(word);
  
  if(reserved_words_iterator == reserved_words.end())
    return false;
  else
    return true;
}

type_t Scanner::get_reserved_type(std::string word)
{
  reserved_words_iterator = reserved_words.find(word);
  return reserved_words_iterator->second;
}

void Scanner::report_warning(std::string message)
{
  std::cerr << "WARNING: " << message;
  num_warnings++;
}

Token* Scanner::get_token(std::ifstream& ifs)
{  
  int i, ch;
  Token* token = NULL;
  
  ch = ifs.get();	// read next char from input stream
  
  while (isblank(ch)) { // if necessary, keep reading til non-space char
    ch = ifs.get();	    // (discard any white space) 
  }

  if (ch == '\n' || ch == '\r') {
    line_number++;
    return get_token(ifs);
  }
  
  if (ch == EOF) {
    token = new Token(TOK_EOF);
  }
  else if (ch >= 0 && ch < 128) {
    if (isdigit(ch)) { // Is a number
      token = new Token(TYPE_INT);
      token->value.int_value = 0;
      while (isdigit(ch) || ch == '.') {
        if (ch == '.') {
          token->type = TYPE_FLOAT;
          break;
        }
        else
          token->value.int_value = token->value.int_value*10 + (ch - '0');
        ch = ifs.get();
      }
      
      if (ch == '.') {
        token->value.float_value = token->value.int_value;
        i = 10;
        ch = ifs.get();
        while (isdigit(ch)) {
          token->value.float_value = token->value.float_value + ((double)(ch - '0'))/(double)i;
          i = i*10;
          ch = ifs.get();
        }
      }
      ifs.unget();
    }
    else if (isalpha(ch)) { // Is an identifier or reserved word
      std::string word;
      ch = tolower(ch);
      while (isalnum(ch) || ch == '_') {
        word += ch;
        ch = tolower(ifs.get());
      }
      ifs.unget();

      if (is_reserved(word)) {
        type_t t = get_reserved_type(word);
        token = new Token(t);
      }
      else {
        token = new Token(TOK_IDENTIFIER);
        token->string = word;
      }
    }
    else if (ch == '"') { // String
      token = new Token(TYPE_STRING);
      ch = ifs.get();
      while (isalnum(ch) || isblank(ch) || ch == '_' || ch == ',' || ch == ';' || ch == ':' || ch == '.' || ch == '\'' || ch == '\\')
      {
        token->string += ch;
        ch = ifs.get();
        if (ch == '"')
          break;
      }

      if (ch != '"') {
        syntax_error("Invalid string constant at line "+std::to_string(line_number)+"\n");
      }
    }
    else if (ch == '!') {
      ch = ifs.get();
      if (ch == '=') {
        token = new Token(TOK_RELOP);
        token->value.relop = NOT_EQUAL;
      }
      else {
        syntax_error("Invalid token at line "+std::to_string(line_number)+"\n");
        return get_token(ifs);
      }
    }
    else if (ch == '=') {
      ch = ifs.get();
      if (ch == '=') {
        token = new Token(TOK_RELOP);
        token->value.relop = IS_EQUAL;
      }
      else {
        syntax_error("Invalid token at line "+std::to_string(line_number)+"\n");
        return get_token(ifs);
      }
    }
    else if (ch == '/') {
      ch = ifs.get();
      if (ch == '/') {
        do {
          ch = ifs.get();
        } while (ch != '\n');
          ifs.unget();
          return get_token(ifs);
      }
      else
        ifs.unget(); token = new Token(TOK_DIVIDE);
    }
    else if (ch == ':') {
      ch = ifs.get();
      if (ch == '=')
        token = new Token(TOK_ASSIGNMENT);
      else {
        ifs.unget();
        token = new Token(TOK_COLON);
      }
    }
    else if (ch == '<') {
      ch = ifs.get();
      if (ch == '=') {
        token = new Token(TOK_RELOP);
        token->value.relop = LESS_OR_EQUAL;
      }
      else {
        ifs.unget(); token = new Token(TOK_RELOP);
        token->value.relop = LESS_THAN;
      }
    }
    else if (ch == '>') {
      ch = ifs.get();
      if (ch == '=') {
        token = new Token(TOK_RELOP);
        token->value.relop = GREATER_OR_EQUAL;
      }
      else {
        ifs.unget(); token = new Token(TOK_RELOP);
        token->value.relop = GREATER_THAN;
      }
    }
    else if (ch == ';') 
      token = new Token(TOK_SEMICOLON);
    else if (ch == ',')
      token = new Token(TOK_COMMA);
    else if (ch == '+')
      token = new Token(TOK_PLUS);
    else if (ch == '-')
      token = new Token(TOK_MINUS);
    else if (ch == '*')
      token = new Token(TOK_MULTIPLY);
    else if (ch == '(')
      token = new Token(TOK_OPEN_PAREN);
    else if (ch == ')')
      token = new Token(TOK_CLOSE_PAREN);
    else if (ch == '{')
      token = new Token(TOK_OPEN_BRACE);
    else if (ch == '}')
      token = new Token(TOK_CLOSE_BRACE);
    else if (ch == '[')
      token = new Token(TOK_OPEN_BRACKET);
    else if (ch == ']')
      token = new Token(TOK_CLOSE_BRACKET);
    else if (ch == '&')
      token = new Token(TOK_AND);
    else if (ch == '|')
      token = new Token(TOK_OR);
    else {
      char* mesg = new char[32];
      sprintf(mesg, "Unknown token '%c' at line %d", ch, line_number);
      syntax_error(mesg);
      delete [] mesg;
      return get_token(ifs);
    }
  }
  else {
    char* mesg = new char[32];
    sprintf(mesg, "Unknown token '%c' at line %d\n", ch, line_number);
    syntax_error(mesg);
    delete [] mesg;
    return get_token(ifs);
  }
  
  return token;
}

void Scanner::syntax_error(std::string mesg)
{
  std::cerr << "ERROR: " << mesg;
  num_errors++;
}

std::string& Scanner::print_token(type_t type, std::string& string)
{  
  std::string squote = "'";
  std::string chara;
  if (type >= 0 && type <= 255) {
        chara = (unsigned char)type;
    string = squote + chara + squote;
  }
  else if (type == TOK_RELOP)
    string = "Relational operator";
  else if (type == TOK_ASSIGNMENT)
    string = "':='";
  else if (type == RESERVED_FALSE)
    string = "'false'";
  else if (type == RESERVED_TRUE)
    string = "'true'";
  else if (type == RESERVED_IS)
    string = "'is'";
  else if (type == RESERVED_STRING)
    string = "'string'";
  else if (type == RESERVED_INT)
    string = "'integer'";
  else if (type == RESERVED_BOOL)
    string = "'bool'";
  else if (type == RESERVED_FLOAT)
    string = "'float'";
  else if (type == RESERVED_GLOBAL)
    string = "'global'";
  else if (type == RESERVED_IN)
    string = "'in'";
  else if (type == RESERVED_OUT)
    string = "'out'";
  else if (type == RESERVED_IF)
    string = "'if'";
  else if (type == RESERVED_THEN)
    string = "'then'";
  else if (type == RESERVED_ELSE)
    string = "'else'";
  else if (type == RESERVED_CASE)
    string = "'case'";
  else if (type == RESERVED_FOR)
    string = "'for'";
  else if (type == RESERVED_AND)
    string = "'and'";
  else if (type == RESERVED_OR)
    string = "'or'";
  else if (type == RESERVED_NOT)
    string = "'not'";
  else if (type == RESERVED_PROGRAM)
    string = "'program'";
  else if (type == RESERVED_PROCEDURE)
    string = "'procedure'";
  else if (type == RESERVED_BEGIN)
    string = "'begin'";
  else if (type == RESERVED_RETURN)
    string = "'return'";
  else if (type == RESERVED_END)
    string = "'end'";
  else if (type == TOK_IDENTIFIER)
    string = "Identifier";
  else if (type == TYPE_INT)
    string = "int";
  else if (type == TYPE_FLOAT)
    string = "float";
  else if (type == TYPE_STRING)
    string = "string";
  else if (type == TYPE_BOOL)
    string = "bool";
  else if (type == TOK_EOF)
    string = "'EOF'";
  
  return string;
}