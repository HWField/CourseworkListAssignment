#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// --- Constants ---
// largest data type program currently supports
#define MAX_BYTES (sizeof(int))
#define MAX_BITS (MAX_BYTES * CHAR_BIT)
// all bits + spaces between nibbles + terminating '\0'
#define OUTPUT_SIZE (MAX_BITS + (MAX_BITS - 1) / 4 + 1)
#define ERROR_MESSAGE "Input error."

// --- Helper Functions ---
void binaryPrint(unsigned long num, int bits, char *output) {
  int position = 0;
  for (int i = bits - 1; i >= 0; i--) {
    if ((num >> i) & 1) {
      output[position] = '1';
    } else {
      output[position] = '0';
    }
    position++;
    // add spacing between each nibble
    if (i % 4 == 0 && i != 0) {
      output[position] = ' ';
      position++;
    }
  }
  output[position] = '\0';
}

bool validInteger(const char *value) {
  size_t i = 0;
  // if negative sign then skip past on subsequent checks
  if (value[i] == '-') {
    i++;
  }
  if (value[i] == '\0') {
    return false;
  }
  // reject leading zeros
  if (value[i] == '0' && value[i + 1] != '\0') {
    return false;
  }
  // check that all characters are digits
  for (; value[i] != '\0'; i++) {
    if (value[i] < '0' || value[i] > '9') {
      return false;
    }
  }
  return true;
}
void inputError(char *output) {
  strcpy(output, ERROR_MESSAGE);
}

// --- conversion functions ---
void convert(const char *type, const char *value, char *output) {
  int outputBits = 0;
  int outputValue = 0;
  // branch between input types
  if (strcmp(type, "char") == 0) {

    outputValue = atoi(value);
    if (!validInteger(value) || outputValue < CHAR_MIN || outputValue > CHAR_MAX) {
      inputError(output);
      return;
    }
    outputBits = sizeof(char) * CHAR_BIT;
  } else {
    inputError(output);
    return;
  }
  binaryPrint(outputValue, outputBits, output);
}

// --- Testing Helper Functions ---

// assert function reused from prior list skeleton code
void assert(int line, bool b) {
  if (b)
    return;
  printf("The test on line %d fails.\n", line);
  exit(1);
}

// Check one decimal to binary test run.
bool check(const char *type, const char *value, const char *expected) {
  char output[OUTPUT_SIZE] = "";
  convert(type, value, output);
  return strcmp(output, expected) == 0;
}

// --- Tests for each variable type ---
void testChar(void) {
  assert(__LINE__, check("char", "7", "0000 0111"));
  assert(__LINE__, check("char", "-128", "1000 0000"));
  assert(__LINE__, check("char", "255", "Input error."));
  assert(__LINE__, check("char", "08", "Input error."));
  assert(__LINE__, check("char", "-x0", "Input error."));
}

void testInt(void) {
  assert(__LINE__, check("int", "10000000", "0000 0000 1001 1000 1001 0110 1000 0000"));
}

void testLong(void) {
  assert(__LINE__, check("long", "10000000000000000", "0<PASSWORD>  <PASSWORD>  <PASSWORD>  <PASSWORD>  <PASSWORD>  <PASSWORD>  <PASSWORD>  <PASSWORD>"));
}

void test(void) {
  testChar();
  // testInt();
  // testLong();
  printf("All tests pass.\n");
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    test();
  } else if (argc == 3) {
    char output[OUTPUT_SIZE] = "";
    convert(argv[1], argv[2], output);
    printf("%s\n", output);
  } else {
    printf(ERROR_MESSAGE "\n");
  }
  return 0;
}
