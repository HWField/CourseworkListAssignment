#include <errno.h>
#include <float.h>
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
void printBinary(unsigned long num, size_t bits, char *output) {
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

bool readBinary(char *values[], size_t bits, unsigned long *result) {
  // check the requested bit width matches provided input
  if (values == NULL || result == NULL || bits == 0 || bits % 4 != 0 || bits > sizeof(unsigned long) * CHAR_BIT) {
    return false;
  }
  size_t nibbles = bits / 4;
  unsigned long number = 0;
  for (size_t i = 0; i < nibbles; i++) {
    // check that each nibble contains exactly four digits
    if (values[i] == NULL || strlen(values[i]) != 4) {
      return false;
    }
    for (size_t j = 0; j < 4; j++) {
      // reject any character other than 0 or 1
      if (values[i][j] != '0' && values[i][j] != '1') {
        return false;
      }
      // shift existing bits left and append the next bit
      number = (number << 1) | (unsigned long)(values[i][j] - '0');
    }
  }
  // reject any additional arguments
  if (values[nibbles] != NULL) {
    return false;
  }
  *result = number;
  return true;
}

// strtl will detect invalid strings however examples say to invalidate leading zeros thus explicitly checked
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

void decimalToBinary(const char *type, const char *value, char *output) {
  // branch between input types
  // initial check for double type as will use seperate processing
  if (strcmp(type, "double") == 0) {
    char *end;
    errno = 0;
    double number = strtod(value, &end);
    // don't check leading zero as e.g. 0.5 still valid which leading zero would reject
    if (end == value || *end != '\0' || errno == ERANGE) {
      inputError(output);
      return;
    }
    unsigned long bits;
    // by using the fact a 64bit long is same length as double can directly copy the double into a long type for printing with existing function
    memcpy(&bits, &number, sizeof(number));
    printBinary(bits, sizeof(double) * CHAR_BIT, output);
    return;
  }
  // these data types must be large enough to hold the largest data type program currently supports
  long typeMaximum;
  long typeMinimum;
  size_t outputBits;
  // branch between integer types
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
  } else if (strcmp(type, "unsigned char") == 0) {
    typeMaximum = UCHAR_MAX;
    typeMinimum = 0;
    outputBits = sizeof(char) * CHAR_BIT;
  } else if (strcmp(type, "unsigned int") == 0) {
    typeMaximum = UINT_MAX;
    typeMinimum = 0;
    outputBits = sizeof(int) * CHAR_BIT;
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
  printBinary(outputValue, outputBits, output);
}

void binaryToDecimal(const char *type, char *values[], char *output) {
  size_t bits;
  // determine the expected number of bits for each type
  if (strcmp(type, "char") == 0 || strcmp(type, "unsigned char") == 0) {
    bits = sizeof(char) * CHAR_BIT;
  } else if (strcmp(type, "int") == 0 || strcmp(type, "unsigned int") == 0) {
    bits = sizeof(int) * CHAR_BIT;
  } else if (strcmp(type, "long") == 0) {
    bits = sizeof(long) * CHAR_BIT;
  } else if (strcmp(type, "double") == 0) {
    bits = sizeof(double) * CHAR_BIT;
  } else {
    inputError(output);
    return;
  }
  // read and validate the binary input
  unsigned long binaryValue;
  if (!readBinary(values, bits, &binaryValue)) {
    inputError(output);
    return;
  }
  // interpret the bit pattern according to the specified type
  // use memcopy to intepret result as correctly (un)signed value
  if (strcmp(type, "char") == 0) {
    unsigned char raw = (unsigned char)binaryValue;
    signed char value;
    memcpy(&value, &raw, sizeof(value));
    sprintf(output, "%hhd", value);
  } else if (strcmp(type, "int") == 0) {
    unsigned int raw = (unsigned int)binaryValue;
    int value;
    memcpy(&value, &raw, sizeof(value));
    sprintf(output, "%d", value);
  } else if (strcmp(type, "long") == 0) {
    long value;
    memcpy(&value, &binaryValue, sizeof(value));
    sprintf(output, "%ld", value);
  } else if (strcmp(type, "unsigned char") == 0) {
    sprintf(output, "%hhu", (unsigned char)binaryValue);
  } else if (strcmp(type, "unsigned int") == 0) {
    sprintf(output, "%u", (unsigned int)binaryValue);
  } else if (strcmp(type, "double") == 0) {
    double value;
    memcpy(&value, &binaryValue, sizeof(value));
    // the DBL_DECIMAL_DIG from float.h defines the max significant figures required to represent a double value
    // this will be maximum 17 figures for a 64 bit double type
    sprintf(output, "%.*g", DBL_DECIMAL_DIG, value);
  }
}

void convert(char *args[], char *output) {
  if (args[0] == NULL) {
    inputError(output);
    return;
  }
  const char *type = args[0];
  char **values = args + 1;
  char unsignedType[sizeof("unsigned char")];
  // combine unsigned with its following type
  if (strcmp(type, "unsigned") == 0) {
    if (args[1] == NULL || (strcmp(args[1], "char") != 0 && strcmp(args[1], "int") != 0)) {
      inputError(output);
      return;
    }
    sprintf(unsignedType, "unsigned %s", args[1]);
    type = unsignedType;
    values = args + 2;
  }
  // the smallest supported binary type is a char which requires two nibbles thus can use argument length to determine the type of covnversion required
  // Count the remaining arguments
  int count = 0;
  while (values[count] != NULL) {
    count++;
  }
  // determine conversion direction
  if (count == 1) {
    decimalToBinary(type, values[0], output);
  } else if (count > 1) {
    binaryToDecimal(type, values, output);
  } else {
    inputError(output);
  }
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
bool check(char *args[], const char *expected) {
  char output[OUTPUT_SIZE] = "";
  convert(args, output);
  return strcmp(output, expected) == 0;
}

// --- Tests for each variable type ---
// These tests assume that Char = 8bit, Int = 32, Long = 64 as is the case on MVB Linux Lab machines used for coursework.
// The tests simulate an argument array which is sent to the convert function in the same way a user interfacing with convert through main / program argumens would
void testChar(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"char", "0", NULL}, "0000 0000"));
  assert(__LINE__, check((char *[]){"char", "7", NULL}, "0000 0111"));
  assert(__LINE__, check((char *[]){"char", "127", NULL}, "0111 1111"));
  assert(__LINE__, check((char *[]){"char", "-1", NULL}, "1111 1111"));
  assert(__LINE__, check((char *[]){"char", "-128", NULL}, "1000 0000"));

  // Out-of-range inputs
  assert(__LINE__, check((char *[]){"char", "128", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "-129", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "255", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"char", "", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "08", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "-", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "12x", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "1.5", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "-x0", NULL}, "Input error."));
}

void testInt(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"int", "0", NULL}, "0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"int", "1", NULL}, "0000 0000 0000 0000 0000 0000 0000 0001"));
  assert(__LINE__, check((char *[]){"int", "-1", NULL}, "1111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check((char *[]){"int", "10000000", NULL}, "0000 0000 1001 1000 1001 0110 1000 0000"));
  assert(__LINE__, check((char *[]){"int", "2147483647", NULL}, "0111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check((char *[]){"int", "-2147483648", NULL}, "1000 0000 0000 0000 0000 0000 0000 0000"));

  // Out-of-range inputs
  assert(__LINE__, check((char *[]){"int", "2147483648", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "-2147483649", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"int", "", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "-", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "12x", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "1.5", NULL}, "Input error."));
}

void testLong(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"long", "0", NULL}, "0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"long", "1", NULL}, "0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001"));
  assert(__LINE__, check((char *[]){"long", "-1", NULL}, "1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111"));

  // Values outside the int range
  assert(__LINE__, check((char *[]){"long", "4294967296", NULL}, "0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"long", "-4294967296", NULL}, "1111 1111 1111 1111 1111 1111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000"));

  // Exact boundaries of long
  assert(__LINE__, check((char *[]){"long", "9223372036854775807", NULL}, "0111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check((char *[]){"long", "-9223372036854775808", NULL}, "1000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));

  // Out-of-range inputs
  assert(__LINE__, check((char *[]){"long", "9223372036854775808", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"long", "-9223372036854775809", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"long", "", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"long", "-", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"long", "12x", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"long", "1.5", NULL}, "Input error."));
}

void testUnsignedChar(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"unsigned", "char", "0", NULL}, "0000 0000"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "1", NULL}, "0000 0001"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "7", NULL}, "0000 0111"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "127", NULL}, "0111 1111"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "128", NULL}, "1000 0000"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "255", NULL}, "1111 1111"));

  // Out-of-range inputs
  assert(__LINE__, check((char *[]){"unsigned", "char", "256", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "-1", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "-128", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "999999", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"unsigned", "char", "", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "08", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "-", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "12x", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "1.5", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "-x0", NULL}, "Input error."));
}

void testUnsignedInt(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"unsigned", "int", "0", NULL}, "0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "1", NULL}, "0000 0000 0000 0000 0000 0000 0000 0001"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "255", NULL}, "0000 0000 0000 0000 0000 0000 1111 1111"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "65535", NULL}, "0000 0000 0000 0000 1111 1111 1111 1111"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "2147483647", NULL}, "0111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "2147483648", NULL}, "1000 0000 0000 0000 0000 0000 0000 0000"));
  // explicit check boundry uint max
  assert(__LINE__, check((char *[]){"unsigned", "int", "4294967295", NULL}, "1111 1111 1111 1111 1111 1111 1111 1111"));

  // Out-of-range inputs
  assert(__LINE__, check((char *[]){"unsigned", "int", "4294967296", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "-1", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "-2147483648", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"unsigned", "int", "", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "08", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "-", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "12x", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "1.5", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "-x0", NULL}, "Input error."));
}

void testDouble(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"double", "0", NULL}, "0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "1", NULL}, "0011 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "-1", NULL}, "1011 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "1.25", NULL}, "0011 1111 1111 0100 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "-1.25", NULL}, "1011 1111 1111 0100 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "0.5", NULL}, "0011 1111 1110 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "-0.0", NULL}, "1000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "2.5", NULL}, "0100 0000 0000 0100 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));

  // Out-of-range inputs
  assert(__LINE__, check((char *[]){"double", "1e309", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "-1e309", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "1e-4000", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"double", "", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "-", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", ".", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "1.2.3", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "12x", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "1e", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "-x0", NULL}, "Input error."));
}

void testBinaryChar(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"char", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"char", "0000", "0111", NULL}, "7"));
  assert(__LINE__, check((char *[]){"char", "0111", "1111", NULL}, "127"));
  assert(__LINE__, check((char *[]){"char", "1111", "1111", NULL}, "-1"));
  assert(__LINE__, check((char *[]){"char", "1000", "0000", NULL}, "-128"));
  assert(__LINE__, check((char *[]){"char", "1000", "0001", NULL}, "-127"));

  // Incorrect number of nibbles
  assert(__LINE__, check((char *[]){"char", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "0000", "0000", "0000", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"char", "000", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "00000", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "0000", "000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "0000", "0002", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "100x", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "", "0000", NULL}, "Input error."));
}

void testBinaryInt(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0001", NULL}, "1"));
  assert(__LINE__, check((char *[]){"int", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "-1"));
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "1001", "1000", "1001", "0110", "1000", "0000", NULL}, "10000000"));
  assert(__LINE__, check((char *[]){"int", "0111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "2147483647"));
  assert(__LINE__, check((char *[]){"int", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-2147483648"));
  assert(__LINE__, check((char *[]){"int", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0001", NULL}, "-2147483647"));

  // Incorrect number of nibbles
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "001", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0002", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "00x1", NULL}, "Input error."));
}

void testBinaryLong(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0001", NULL}, "1"));
  assert(__LINE__, check((char *[]){"long", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "-1"));

  // Values outside the int range
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0001", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "4294967296"));
  assert(__LINE__, check((char *[]){"long", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-4294967296"));

  // Exact boundaries of long
  assert(__LINE__, check((char *[]){"long", "0111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "9223372036854775807"));
  assert(__LINE__, check((char *[]){"long", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-9223372036854775808"));

  // Incorrect number of nibbles
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "001", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "000x", NULL}, "Input error."));
}

void testBinaryUnsignedChar(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0001", NULL}, "1"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0111", NULL}, "7"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0111", "1111", NULL}, "127"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "1000", "0000", NULL}, "128"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "1111", "1111", NULL}, "255"));

  // Incorrect number of nibbles
  assert(__LINE__, check((char *[]){"unsigned", "char", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0000", "0000", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"unsigned", "char", "000", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "00000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0002", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "111x", "1111", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "", "0000", NULL}, "Input error."));
}

void testBinaryUnsignedInt(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0001", NULL}, "1"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "1111", "1111", NULL}, "255"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "1111", "1111", "1111", "1111", NULL}, "65535"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "0111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "2147483647"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "2147483648"));

  // Exact boundary of unsigned int
  assert(__LINE__, check((char *[]){"unsigned", "int", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "4294967295"));

  // Incorrect number of nibbles
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "001", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0002", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "00x1", NULL}, "Input error."));
}

void testBinaryDouble(void) {
  // Expected inputs
  assert(__LINE__, check((char *[]){"double", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"double", "0011", "1111", "1111", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "1"));
  assert(__LINE__, check((char *[]){"double", "1011", "1111", "1111", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-1"));
  assert(__LINE__, check((char *[]){"double", "0011", "1111", "1111", "0100", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "1.25"));
  assert(__LINE__, check((char *[]){"double", "1011", "1111", "1111", "0100", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-1.25"));
  assert(__LINE__, check((char *[]){"double", "0011", "1111", "1110", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "0.5"));
  assert(__LINE__, check((char *[]){"double", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-0"));
  assert(__LINE__, check((char *[]){"double", "0100", "0000", "0000", "0100", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "2.5"));

  // Check significant digit precision
  assert(__LINE__, check((char *[]){"double", "0011", "1111", "1011", "1001", "1001", "1001", "1001", "1001", "1001", "1001", "1001", "1001", "1001", "1001", "1001", "1010", NULL}, "0.10000000000000001"));

  // Incorrect number of nibbles
  assert(__LINE__, check((char *[]){"double", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));

  // Malformed inputs
  assert(__LINE__, check((char *[]){"double", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "000x", NULL}, "Input error."));
}

void testBinaryInvalidType(void) {
  assert(__LINE__, check((char *[]){"short", "0000", "0111", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"", "0000", "0111", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "short", "0000", "0111", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", NULL}, "Input error."));
}

void testInvalidType(void) {
  assert(__LINE__, check((char *[]){"short", "7", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"", "7", NULL}, "Input error."));
}

void test(void) {
  testChar();
  testInt();
  testLong();
  testUnsignedChar();
  testUnsignedInt();
  testDouble();
  testBinaryChar();
  testBinaryInt();
  testBinaryLong();
  testBinaryUnsignedChar();
  testBinaryUnsignedInt();
  testBinaryDouble();
  testBinaryInvalidType();
  testInvalidType();
  printf("All tests pass.\n");
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    test();
    return 0;
  }
  char output[OUTPUT_SIZE] = "";
  convert(argv + 1, output);
  printf("%s\n", output);
  return 0;
}
