// Main.cpp
#include <iostream>
#include <cstdlib>
#include "parser.h"

int main(int argc, char** argv)
{
  char* filename;
  
  if(argc > 1)
    filename = argv[1];
  else {
    std::cout << "Missing parameter: filename" << std::endl;
    exit(1);
  }
  
  Parser* parser = new Parser(filename);
  parser->start();

  if (Scanner::num_errors > 0 && !(parser->EOF_found))
    std::cout << "Compilation terminated with: \n\t" << Scanner::num_errors << " Errors, \t" << Scanner::num_warnings << " Warnings\n"; 
  else if (Scanner::num_errors > 0)
    std::cout << "Parsing complete, compilation terminated with: \n\t" << Scanner::num_errors << " Errors, \t" << Scanner::num_warnings << " Warnings\n";
  else
    std::cout << "Compilation completed with: \n\t" << Scanner::num_errors << " Errors, \t" << Scanner::num_warnings << " Warnings\n"; 
  
  delete parser;

  return 0;
}