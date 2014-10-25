#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_REGS 32
#define MEM_SIZE 1024*1024
long long Reg[NUM_REGS];
long long MM[MEM_SIZE];
#define SP NUM_REGS-1
#define FP NUM_REGS-2

long long heap_pointer;
long long heapAlloc(long long size);

bool getBool()
{
  unsigned b;
  scanf("%u", &b);
  
  if (b) return true;
  else return false;
}

int getInteger()
{
  int i;
  scanf("%d", &i);
  return i;
}

float getFloat()
{
  float f;
  scanf("%f", &f);
  return f;
}

char* getString()
{
  long long address = heapAlloc(8); // 64 bytes
  scanf("%s", (char*)&MM[address]);
  return (char*)&MM[address];
}

int putBool(bool b)
{
  if(b)
    printf("true");
  else
    printf("false");
  
  return 0;
}

int putInteger(int i)
{
  printf("%d", i);
  return 0;
}

int putFloat(float f)
{
  printf("%f", f);
  return 0;
}

int putString(char* s)
{
  printf("%s", s);
  return 0;
}

void init(long long start_address)
{
  Reg[SP] = MEM_SIZE;
  Reg[FP] = MEM_SIZE;
  heap_pointer = start_address;
}

long long heapAlloc(long long size)
{
  long long ret = heap_pointer;
  heap_pointer += size;
  return ret;
}

void dataConversionCheck(long long num)
{
  if (num != false && num != true)
    printf("Runtime data conversion error: Integer cannot be compared to a boolean value\n");
}
