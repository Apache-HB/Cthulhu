/**
 * Autogenerated by the Cthulhu Compiler Collection C99 backend
 * Generated from mnt::c::Users::elliot::Documents::GitHub::ctulang::tests::ctu::pass
 */
#include <stddef.h>
#include <stdlib.h>

// String literals
// Imported symbols

// Global forwarding

// Function forwarding
signed int other(signed int arg0);
signed int main(signed int arg0);

// Global initialization

// Function definitions
signed int other(signed int arg0) {
  signed int vreg0 = (signed int)20 * arg0;
  signed int vreg1 = (signed int)vreg0;
  return vreg1;
}
signed int main(signed int arg0) {
  signed int (*o64[1])(signed int);
  *o64 = other;
  signed int vreg1 = (*o64)(arg0);
  return vreg1;
}
