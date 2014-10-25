#include "symbol.h"

Symbol::Symbol(std::string nm, bool global, bool isarray, type_t typ, unsigned int size)
{ // Constructor for variable
  name = nm;
  id_type = ID_VARIABLE;
  is_global = global;
  symbol_type.set_type(typ);
  symbol_type.set_array(isarray);
  symbol_type.set_arraysize(size);
  width = size;
  used = false;
  initialized = false;
  direction = DIRECTION_UNSPECIFIED;
  params = NULL;
  next = NULL;
  address = 0;
  ref_count = 0;
  line_declared = 0;
}

Symbol::Symbol(std::string nm, bool isarray, type_t typ, unsigned int size)
{  // Constructor for parameter
  name = nm;
  id_type = ID_PARAMETER;
  is_global = false;
  symbol_type.set_type(typ);
  symbol_type.set_array(isarray);
  symbol_type.set_arraysize(size);
  width = size;
  used = false;
  initialized = false;
  direction = DIRECTION_UNSPECIFIED;
  params = NULL;
  next = NULL;
  address = 0;
  ref_count = 0;
  line_declared = 0;
}

Symbol::Symbol(std::string nm, bool global)
{ // Constructor for procedure
  name = nm;
  id_type = ID_PROCEDURE;
  is_global = global;
  symbol_type.set_array(false);
  symbol_type.set_arraysize(1);
  used = false;
  initialized = false;
  direction = DIRECTION_UNSPECIFIED;
  params = NULL;
  next = NULL;
  address = 0;
  ref_count = 0;
  line_declared = 0;
}

Type::Type(type_t t, bool array, unsigned int size)
{ 
  variable_type = t;
  is_array = array;
  array_size = size;
  symbol = false;
}

Type::Type(const Type& type)
{
  is_array = type.is_array;
  variable_type = type.variable_type;
  array_size = array_size;
}
    
Type& Type::operator=(const Type& type)
{
  is_array = type.is_array;
  variable_type = type.variable_type;
  array_size = array_size;
  return *this;
}
      
bool Type::operator==(const Type& type)
{
  return ((is_array == type.is_array) && (variable_type == type.variable_type) && (array_size == type.array_size));
}

bool Type::operator!=(const Type& type) 
{
  return !(*this == type);
}
    