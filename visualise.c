#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// --- Constants ---
// largest data type program currently supports
#define MAX_BYTES (sizeof(long))
#define MAX_BITS (MAX_BYTES * CHAR_BIT)
// all bits + spaces between nibbles + terminating '\0'
#define OUTPUT_SIZE (MAX_BITS + (MAX_BITS - 1) / 4 + 1)
#define ERROR_MESSAGE "Input error."

// --- Helper Functions ---
// using unsigned long to benefit from c's auto unwrapping of negative signed long via modulo ULONG_MAX + 1
void binaryPrint(unsigned long num, size_t bits, char *output) {
  int position = 0;
  for (size_t i = bits - 1; i >= 0; i--) {
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
// strtl will detect invalid strings however examples in instructions say to invalidate leading zeros thus explicitly checked
bool hasLeadingZero(const char *value) {
  const char *digits = value;
  if (digits[0] == '-') {
    digits++;
  }
  return digits[0] == '0' && digits[1] != '\0';
}
void inputError(char *output) {
  strcpy(output, ERROR_MESSAGE);
}

// --- conversion functions ---
void convert(const char *type, const char *value, char *output) {
  // these data types must be large enough to hold the largest data type program currently supports
  long typeMaximum;
  long typeMinimum;
  size_t outputBits;

  // branch between input types
  if (strcmp(type, "char") == 0) {
    typeMaximum = CHAR_MAX;
    typeMinimum = CHAR_MIN;
    outputBits = sizeof(char) * CHAR_BIT;
  } else if (strcmp(type, "int") == 0) {
    typeMaximum = INT_MAX;
    typeMinimum = INT_MIN;
    outputBits = sizeof(int) * CHAR_BIT;
  } else if (strcmp(type, "long") == 0) {
    typeMaximum = LONG_MAX;
    typeMinimum = LONG_MIN;
    outputBits = sizeof(long) * CHAR_BIT;
  } else {
    inputError(output);
    return;
  }

  // using strtl here rather than atoi so that if a value larger than LONG_MAX is entered that would overflow is detected and can be parsed as an error rather than incorrectly overflowing, detecting if the value lies within accepted values of the type specified is then handled after char to integer conversion set errno to 0 before use as according to man documentation "Since strtol() can legitimately return 0, LONG_MAX, or LONG_MIN on both success and failure, the calling program should set errno to 0 before the call, and then determine if an error occurred by checking whether errno == ERANGE after the call."
  errno = 0;
  char *end;
  // use end to represent the position that conversion stoppped to find if the number is outside the bounds of long which is largest format supported
  long outputValue = strtol(value, &end, 10);
  if (hasLeadingZero(value) || errno == ERANGE || end == value || *end != '\0' || outputValue < typeMinimum || outputValue > typeMaximum) {
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
// These tests assume that Char = 8bit, Int = 32, Long = 64 as seems to be implied by example
void testChar(void) {
  // Expected inputs
  assert(__LINE__, check("char", "0", "0000 0000"));
  assert(__LINE__, check("char", "7", "0000 0111"));
  assert(__LINE__, check("char", "127", "0111 1111"));
  assert(__LINE__, check("char", "-1", "1111 1111"));
  assert(__LINE__, check("char", "-128", "1000 0000"));

  // Out-of-range inputs
  assert(__LINE__, check("char", "128", "Input error."));
  assert(__LINE__, check("char", "-129", "Input error."));
  assert(__LINE__, check("char", "255", "Input error."));

  // Malformed inputs
  assert(__LINE__, check("char", "", "Input error."));
  assert(__LINE__, check("char", "08", "Input error."));
  assert(__LINE__, check("char", "-", "Input error."));
  assert(__LINE__, check("char", "12x", "Input error."));
  assert(__LINE__, check("char", "1.5", "Input error."));
  assert(__LINE__, check("char", "-x0", "Input error."));
}

void testInt(void) {
  // Expected inputs
  assert(__LINE__, check("int", "0", "0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check("int", "1", "0000 0000 0000 0000 0000 0000 0000 0001"));
  assert(__LINE__, check("int", "-1", "1111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check("int", "10000000", "0000 0000 1001 1000 1001 0110 1000 0000"));
  assert(__LINE__, check("int", "2147483647", "0111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check("int", "-2147483648", "1000 0000 0000 0000 0000 0000 0000 0000"));

  // Out-of-range inputs
  assert(__LINE__, check("int", "2147483648", "Input error."));
  assert(__LINE__, check("int", "-2147483649", "Input error."));

  // Malformed inputs
  assert(__LINE__, check("int", "", "Input error."));
  assert(__LINE__, check("int", "-", "Input error."));
  assert(__LINE__, check("int", "12x", "Input error."));
  assert(__LINE__, check("int", "1.5", "Input error."));
}

void testLong(void) {
  // Expected inputs
  assert(__LINE__, check("long", "0", "0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check("long", "1", "0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001"));
  assert(__LINE__, check("long", "-1", "1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111"));

  // Values outside the int range
  assert(__LINE__, check("long", "4294967296", "0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check("long", "-4294967296", "1111 1111 1111 1111 1111 1111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"));

  // Exact boundaries of long
  assert(__LINE__, check("long", "9223372036854775807", "0111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check("long", "-9223372036854775808", "1000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));

  // Out-of-range inputs
  assert(__LINE__, check("long", "9223372036854775808", "Input error."));
  assert(__LINE__, check("long", "-9223372036854775809", "Input error."));

  // Malformed inputs
  assert(__LINE__, check("long", "", "Input error."));
  assert(__LINE__, check("long", "-", "Input error."));
  assert(__LINE__, check("long", "12x", "Input error."));
  assert(__LINE__, check("long", "1.5", "Input error."));
}

void testInvalidType(void) {
  assert(__LINE__, check("short", "7", "Input error."));
  assert(__LINE__, check("", "7", "Input error."));
}

void test(void) {
  testChar();
  testInt();
  testLong();
  testInvalidType();
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
