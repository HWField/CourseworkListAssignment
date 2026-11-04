#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
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
int argumentCount(char *args[]) {
  int count = 0;
  if (args != NULL) {
    while (args[count] != NULL)
      count++;
  }
  return count;
}

int typeSize(const char *type) {
  if (type == NULL)
    return 0;
  if (strcmp(type, "char") == 0 || strcmp(type, "unsigned char") == 0) {
    return sizeof(char) * CHAR_BIT;
  }
  if (strcmp(type, "int") == 0 || strcmp(type, "unsigned int") == 0) {
    return sizeof(int) * CHAR_BIT;
  }
  if (strcmp(type, "long") == 0)
    return sizeof(long) * CHAR_BIT;
  if (strcmp(type, "double") == 0)
    return sizeof(double) * CHAR_BIT;
  return 0;
}

// helper function to allocate output space large enough to fit (potentially) multiple structured outputs.
char *generateOutputBuffer(char *args[]) {
  int count = argumentCount(args);
  if (count == 0)
    count = 1;
  int capacity = count * OUTPUT_SIZE;
  // ensure output is large enough to return error message
  if (capacity < sizeof(ERROR_MESSAGE))
    capacity = sizeof(ERROR_MESSAGE);
  return malloc(capacity);
}

// using unsigned long to benefit from c's auto unwrapping of negative signed long via modulo ULONG_MAX + 1
void printBinary(unsigned long num, int bits, char *output) {
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

bool readBinary(char *values[], int count, int bits, unsigned long *result) {
  // check the requested bit width matches provided input
  if (values == NULL || result == NULL || bits == 0 || bits % 4 != 0 || bits > sizeof(unsigned long) * CHAR_BIT) {
    return false;
  }
  int nibbles = bits / 4;
  if (count != nibbles)
    return false;
  unsigned long number = 0;
  for (int i = 0; i < nibbles; i++) {
    // check that each nibble contains exactly four digits
    if (values[i] == NULL || strlen(values[i]) != 4) {
      return false;
    }
    for (int j = 0; j < 4; j++) {
      // reject any character other than 0 or 1
      if (values[i][j] != '0' && values[i][j] != '1') {
        return false;
      }
      // shift existing bits left and append the next bit
      number = (number << 1) | (unsigned long)(values[i][j] - '0');
    }
  }
  *result = number;
  return true;
}

// strtol will detect invalid strings however examples say to invalidate leading zeros thus explicitly checked
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

void appendOutput(char *output, int *used, const char *field) {
  int length = strlen(field);
  if (*used != 0)
    output[(*used)++] = ' ';
  memcpy(output + *used, field, length + 1);
  *used += length;
}
// variable to hold the parsed structure information, including the original text, an array of type strings and the number of fields and arguments
typedef struct {
  char *text;
  char **types;
  int count;
  int arguments;
} structure;

// find the closing brace and count space for the type text and final '\0'
int structureSize(char *args[], structure *fields) {
  int textSize = 1;
  bool closed = false;
  while (args[fields->arguments] != NULL && !closed) {
    char *word = args[fields->arguments];
    if (fields->arguments != 0) {
      textSize++;
    }
    for (int i = 0; word[i] != '\0'; i++) {
      textSize++;
      if (word[i] == '}') {
        // the closing brace will be last character of the argument
        if (word[i + 1] != '\0')
          return 0;
        closed = true;
      }
    }
    fields->arguments++;
  }
  if (!closed)
    return 0;
  return textSize;
}

// condense type arguments into one string, putting spaces between words
bool copyStructureText(char *args[], structure *fields, int textSize) {
  fields->text = malloc(textSize);
  if (fields->text == NULL)
    return false;
  int position = 0;
  for (int i = 0; i < fields->arguments; i++) {
    if (i != 0)
      fields->text[position++] = ' ';
    for (int j = 0; args[i][j] != '\0'; j++) {
      fields->text[position] = args[i][j];
      position++;
    }
  }
  // replace the closing brace with the null terminator
  fields->text[position - 1] = '\0';
  return true;
}

// count the fields and reject extra braces inside the structure
int structureFieldCount(const char *text) {
  int fieldCount = 1;
  for (int i = 1; text[i] != '\0'; i++) {
    if (text[i] == '{' || text[i] == '}')
      return 0;
    if (text[i] == ';')
      fieldCount++;
  }
  return fieldCount;
}

// check for whitespace before or after type name
bool isWhitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

// split the type text at semicolons and check every type is supported
bool parseStructureTypes(structure *fields, int fieldCount) {
  fields->types = malloc(fieldCount * sizeof(char *));
  if (fields->types == NULL)
    return false;

  // start after '{' and find each type's start and end
  int position = 1;
  for (int i = 0; i < fieldCount; i++) {
    int start = position;
    while (fields->text[position] != ';' && fields->text[position] != '\0') {
      position++;
    }
    int end = position;
    position++;

    // skip whitespace before the type name
    while (isWhitespace(fields->text[start])) {
      start++;
    }

    // move back over whitespace keeping spaces inside unsigned types
    while (end > start && isWhitespace(fields->text[end - 1])) {
      end--;
    }
    fields->text[end] = '\0';
    char *type = &fields->text[start];
    if (typeSize(type) == 0)
      return false;
    fields->types[i] = type;
  }
  fields->count = fieldCount;
  return true;
}

// read structure arguments and store the parsed types and their count
bool parseStructure(char *args[], structure *fields) {
  fields->text = NULL;
  fields->types = NULL;
  fields->count = 0;
  fields->arguments = 0;
  if (args == NULL || args[0] == NULL || args[0][0] != '{')
    return false;
  int textSize = structureSize(args, fields);
  if (textSize == 0)
    return false;
  if (!copyStructureText(args, fields, textSize))
    return false;
  int fieldCount = structureFieldCount(fields->text);
  if (fieldCount == 0)
    return false;
  return parseStructureTypes(fields, fieldCount);
}

// --- conversion functions ---

bool decimalToBinary(const char *type, const char *value, char *output) {
  int outputBits = typeSize(type);
  if (outputBits == 0 || outputBits > MAX_BITS || value == NULL || output == NULL)
    return false;
  // branch between input types
  // initial check for double type as will use seperate processing
  if (strcmp(type, "double") == 0) {
    char *end;
    errno = 0;
    double number = strtod(value, &end);
    // don't check leading zero as e.g. 0.5 still valid which leading zero would reject
    if (end == value || *end != '\0' || errno == ERANGE || isfinite(number) == 0) {
      return false;
    }
    unsigned long bits = 0;
    // by using the fact a 64bit long is same length as double can directly copy the double into a long type for printing with existing function
    memcpy(&bits, &number, sizeof(number));
    printBinary(bits, outputBits, output);
    return true;
  }
  long typeMaximum;
  long typeMinimum;
  // branch between integer types
  if (strcmp(type, "char") == 0) {
    typeMaximum = CHAR_MAX;
    typeMinimum = CHAR_MIN;
  } else if (strcmp(type, "int") == 0) {
    typeMaximum = INT_MAX;
    typeMinimum = INT_MIN;
  } else if (strcmp(type, "long") == 0) {
    typeMaximum = LONG_MAX;
    typeMinimum = LONG_MIN;
  } else if (strcmp(type, "unsigned char") == 0) {
    typeMaximum = UCHAR_MAX;
    typeMinimum = 0;
  } else if (strcmp(type, "unsigned int") == 0) {
    typeMaximum = UINT_MAX;
    typeMinimum = 0;
  } else {
    return false;
  }
  // using strtol here rather than atoi so that if a value larger than LONG_MAX is entered that would overflow is detected and can be parsed as an error rather than incorrectly overflowing, detecting if the value lies within accepted values of the type specified is then handled after char to integer conversion set errno to 0 before use as according to man documentation "Since strtol() can legitimately return 0, LONG_MAX, or LONG_MIN on both success and failure, the calling program should set errno to 0 before the call, and then determine if an error occurred by checking whether errno == ERANGE after the call."
  errno = 0;
  char *end;
  // use end to represent the position that conversion stoppped using an ERANGE value to determine if the number is outside the bounds of long which is largest format supported
  long outputValue = strtol(value, &end, 10);
  if (hasLeadingZero(value) || errno == ERANGE || end == value || *end != '\0' || outputValue < typeMinimum || outputValue > typeMaximum) {
    return false;
  }
  printBinary(outputValue, outputBits, output);
  return true;
}

bool binaryToDecimal(const char *type, char *values[], int count, char *output) {
  int bits = typeSize(type);
  if (bits == 0 || output == NULL)
    return false;
  // read and validate the binary input
  unsigned long binaryValue;
  if (!readBinary(values, count, bits, &binaryValue))
    return false;
  // interpret the bit pattern according to the specified type
  // use memcopy to intepret result as correctly (un)signed value
  int length;
  if (strcmp(type, "char") == 0) {
    unsigned char raw = (unsigned char)binaryValue;
    signed char value;
    memcpy(&value, &raw, sizeof(value));
    length = sprintf(output, "%hhd", value);
  } else if (strcmp(type, "int") == 0) {
    unsigned int raw = (unsigned int)binaryValue;
    int value;
    memcpy(&value, &raw, sizeof(value));
    length = sprintf(output, "%d", value);
  } else if (strcmp(type, "long") == 0) {
    long value;
    memcpy(&value, &binaryValue, sizeof(value));
    length = sprintf(output, "%ld", value);
  } else if (strcmp(type, "unsigned char") == 0) {
    length = sprintf(output, "%hhu", (unsigned char)binaryValue);
  } else if (strcmp(type, "unsigned int") == 0) {
    length = sprintf(output, "%u", (unsigned int)binaryValue);
  } else if (strcmp(type, "double") == 0) {
    double value;
    memcpy(&value, &binaryValue, sizeof(value));
    // the DBL_DECIMAL_DIG from float.h defines the max significant figures required to represent a double value
    // this will be maximum 17 figures for a 64 bit double type
    length = sprintf(output, "%.*g", DBL_DECIMAL_DIG, value);
  } else {
    return false;
  }
  return length >= 0 && (int)length < OUTPUT_SIZE;
}

void convert(char *arguments[], char *outputBuffer) {
  outputBuffer[0] = '\0';
  if (arguments == NULL || arguments[0] == NULL) {
    inputError(outputBuffer);
    return;
  }

  char *type = arguments[0];
  int fieldCount = 1;
  int firstValue = 1;
  bool structuredInput = type[0] == '{';
  structure structureInfo = {0};
  bool valid = true;
  char unsignedType[sizeof("unsigned char")];
  if (structuredInput) {
    valid = parseStructure(arguments, &structureInfo);
    if (valid) {
      fieldCount = (int)structureInfo.count;
      firstValue = (int)structureInfo.arguments;
    }
  } else if (strcmp(type, "unsigned") == 0) {
    // combine the two words in an unsigned type
    if (arguments[1] == NULL || (strcmp(arguments[1], "char") != 0 && strcmp(arguments[1], "int") != 0)) {
      valid = false;
    } else {
      sprintf(unsignedType, "unsigned %s", arguments[1]);
      type = unsignedType;
      firstValue = 2;
    }
  }

  // count the binary nibbles needed for all fields
  int expectedBinaryGroups = 0;
  for (int fieldIndex = 0; valid && fieldIndex < fieldCount; fieldIndex++) {
    // use the single type unless this input describes a structure
    char *fieldType = type;
    if (structuredInput) {
      fieldType = structureInfo.types[fieldIndex];
    }
    int fieldBits = (int)typeSize(fieldType);
    expectedBinaryGroups += fieldBits / 4;
  }
  // one value per field means decimal input
  int inputCount = (int)argumentCount(arguments) - firstValue;
  bool decimalInput = inputCount == fieldCount;
  // if the input is binary, the number of nibbles must match the expected number of nibbles for the fields
  if (!decimalInput && inputCount != expectedBinaryGroups)
    valid = false;
  // convert each field and append its output
  int inputIndex = firstValue;
  int outputLength = 0;
  for (int fieldIndex = 0; valid && fieldIndex < fieldCount; fieldIndex++) {
    char *fieldType = type;
    if (structuredInput) {
      fieldType = structureInfo.types[fieldIndex];
    }
    char fieldOutput[OUTPUT_SIZE];
    int inputsForField = decimalInput ? 1 : (int)typeSize(fieldType) / 4;
    if (decimalInput) {
      valid = decimalToBinary(fieldType, arguments[inputIndex], fieldOutput);
    } else {
      valid = binaryToDecimal(fieldType, &arguments[inputIndex], inputsForField, fieldOutput);
    }
    if (valid) {
      appendOutput(outputBuffer, &outputLength, fieldOutput);
    }
    inputIndex += inputsForField;
  }

  // free any memory allocated when parsing a structure
  free(structureInfo.types);
  free(structureInfo.text);
  if (!valid)
    inputError(outputBuffer);
}

// --- Testing Helper Functions ---
// assert function reused from prior list skeleton code
void assert(int line, bool b) {
  if (b)
    return;
  printf("The test on line %d fails.\n", line);
  exit(1);
}

// Check one conversion
bool check(char *args[], const char *expected) {
  char *outputBuffer = generateOutputBuffer(args);
  if (outputBuffer == NULL)
    return false;
  convert(args, outputBuffer);
  bool matches = strcmp(outputBuffer, expected) == 0;
  free(outputBuffer);
  return matches;
}

// --- Tests for each variable type ---
// These tests assume that Char = 8bit, Int = 32, Long = 64 as is the case on MVB Linux Lab machines specified for coursework.
// The tests simulate an argument array which is sent to the convert function in the same way a user interfacing with convert through main / program argumens would
void testDecimalInput(void) {
  // char
  assert(__LINE__, check((char *[]){"char", "5", NULL}, "0000 0101"));
  assert(__LINE__, check((char *[]){"char", "-128", NULL}, "1000 0000"));
  assert(__LINE__, check((char *[]){"char", "0", NULL}, "0000 0000"));
  assert(__LINE__, check((char *[]){"char", "125", NULL}, "0111 1101"));

  // int
  assert(__LINE__, check((char *[]){"int", "10000000", NULL}, "0000 0000 1001 1000 1001 0110 1000 0000"));
  assert(__LINE__, check((char *[]){"int", "-2147483648", NULL}, "1000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"int", "2147483647", NULL}, "0111 1111 1111 1111 1111 1111 1111 1111"));

  // long
  assert(__LINE__, check((char *[]){"long", "4294967296", NULL}, "0000 0000 0000 0000 0000 0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"long", "-9223372036854775808", NULL}, "1000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"long", "9223372036854775807", NULL}, "0111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111"));

  // unsigned char
  assert(__LINE__, check((char *[]){"unsigned", "char", "0", NULL}, "0000 0000"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "255", NULL}, "1111 1111"));

  // unsigned int
  assert(__LINE__, check((char *[]){"unsigned", "int", "0", NULL}, "0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "4294967295", NULL}, "1111 1111 1111 1111 1111 1111 1111 1111"));

  // double
  assert(__LINE__, check((char *[]){"double", "-1.25", NULL}, "1011 1111 1111 0100 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "0.0", NULL}, "0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "-0.0", NULL}, "1000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
  assert(__LINE__, check((char *[]){"double", "-1.7976931348623157e308", NULL}, "1111 1111 1110 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check((char *[]){"double", "1.7976931348623157e308", NULL}, "0111 1111 1110 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111"));
  assert(__LINE__, check((char *[]){"double", "2.2250738585072014e-308", NULL}, "0000 0000 0001 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));
}

void testBinaryInput(void) {
  // char
  assert(__LINE__, check((char *[]){"char", "1000", "0000", NULL}, "-128"));
  assert(__LINE__, check((char *[]){"char", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"char", "0111", "1000", NULL}, "120"));

  // int
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "1001", "1000", "1001", "0110", "1000", "0000", NULL}, "10000000"));
  assert(__LINE__, check((char *[]){"int", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-2147483648"));
  assert(__LINE__, check((char *[]){"int", "0111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "2147483647"));

  // long
  assert(__LINE__, check((char *[]){"long", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0001", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "4294967296"));
  assert(__LINE__, check((char *[]){"long", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-9223372036854775808"));
  assert(__LINE__, check((char *[]){"long", "0111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "9223372036854775807"));

  // unsigned char
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0111", NULL}, "7"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"unsigned", "char", "1111", "1111", NULL}, "255"));

  // unsigned int
  assert(__LINE__, check((char *[]){"unsigned", "int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"unsigned", "int", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "4294967295"));

  // double
  assert(__LINE__, check((char *[]){"double", "1011", "1111", "1111", "0100", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-1.25"));
  assert(__LINE__, check((char *[]){"double", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "0"));
  assert(__LINE__, check((char *[]){"double", "1000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "-0"));
  assert(__LINE__, check((char *[]){"double", "1111", "1111", "1110", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "-1.7976931348623157e+308"));
  assert(__LINE__, check((char *[]){"double", "0111", "1111", "1110", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", "1111", NULL}, "1.7976931348623157e+308"));
  assert(__LINE__, check((char *[]){"double", "0000", "0000", "0001", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "2.2250738585072014e-308"));
}

void testInvalidInput(void) {
  // char
  assert(__LINE__, check((char *[]){"char", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "08", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "-x0", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "0000", "0002", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "-129", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"char", "128", NULL}, "Input error."));

  // int
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "1001", "1000", "1001", "0110", "1000", "000", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "-2147483649", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "2147483648", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"int", "0000", "0000", "0000", "0000", "0000", "0000", "0000", NULL}, "Input error."));

  // long
  assert(__LINE__, check((char *[]){"long", "9223372036854775808", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"long", "-9223372036854775809", NULL}, "Input error."));

  // unsigned char
  assert(__LINE__, check((char *[]){"unsigned", "char", "-1", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "256", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "char", "0000", "0000", "0000", NULL}, "Input error."));

  // unsigned int
  assert(__LINE__, check((char *[]){"unsigned", "int", "-1", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"unsigned", "int", "4294967296", NULL}, "Input error."));

  // double
  assert(__LINE__, check((char *[]){"double", "1e309", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "-1e309", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"double", "1e-4000", NULL}, "Input error."));

  // Unsupported type
  assert(__LINE__, check((char *[]){"short", "7", NULL}, "Input error."));
}

void testStructuredInput(void) {
  // both assignment examples
  const char *example = "0000 0111 0000 0000 1001 1000 1001 0110 1000 0000 1111 1111";
  assert(__LINE__, check((char *[]){"{char;int;unsigned char}", "7", "10000000", "255", NULL}, example));
  assert(__LINE__, check((char *[]){"{char;int;unsigned char}", "0000", "0111", "0000", "0000", "1001", "1000", "1001", "0110", "1000", "0000", "1111", "1111", NULL}, "7 10000000 255"));
  assert(__LINE__, check((char *[]){"{char;int;unsigned", "char}", "7", "10000000", "255", NULL}, example));

  // a valid structure whose output exceeds one scalar buffer.
  assert(__LINE__, check((char *[]){"{long;double}", "-1", "1.25", NULL}, "1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 1111 "
                                                                          "0011 1111 1111 0100 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000"));

  // malformed types as an invalid value in a field and mixed input.
  assert(__LINE__, check((char *[]){"{char;int", "7", "1", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"{char;;int}", "7", "1", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"{char;int;unsigned char}", "7", "10000000", "256", NULL}, "Input error."));
  assert(__LINE__, check((char *[]){"{char;char}", "7", "0000", "0111", NULL}, "Input error."));
}

void test(void) {
  testDecimalInput();
  testBinaryInput();
  testInvalidInput();
  testStructuredInput();
  printf("All tests pass.\n");
}
int main(int argc, char *argv[]) {
  if (argc == 1) {
    test();
    return 0;
  }
  // skip program name (first argument) and pass the rest to convert
  char *outputBuffer = generateOutputBuffer(argv + 1);
  convert(argv + 1, outputBuffer);
  printf("%s\n", outputBuffer);
  free(outputBuffer);
  return 0;
}
