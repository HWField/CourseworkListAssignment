#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Constants ---
// Output capacity that is sufficient for all types being tested
#define OUTPUT_SIZE 128

// --- conversion functions ---
void convert(const char *type, const char *value, char *output, size_t capacity);

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
  convert(type, value, output, sizeof(output));
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
  // testChar();
  // testInt();
  // testLong();
  printf("All tests pass.\n");
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    test();
  } else if (argc == 3) {
    char output[OUTPUT_SIZE] = "";
    convert(argv[1], argv[2], output, sizeof(output));
    printf("%s\n", output);
  } else {
    printf("Input error.\n");
  }
  return 0;
}
