/**
 * Autogenerated by the Cthulhu Compiler Collection C99 backend
 * Generated from mnt::c::Users::ehb56::OneDrive::Documents::GitHub::ctulang::tests::ctu::pass
 */
#include <stddef.h>

// String literals
// Imported symbols

// Global forwarding

// Function forwarding
signed int main();
signed int fac(signed int arg0);

// Global initialization

// Function definitions
signed int main() {
  signed int vreg0 = (*&fac)((signed int)5);
  return vreg0;
}
signed int fac(signed int arg0) {
  signed int acc54;
  signed int j74;
  *&acc54 = (signed int)1;
  *&j74 = (signed int)1;
block2: /* empty */;
  signed int vreg3 = *&j74;
  _Bool vreg4 = vreg3 <= arg0;
  if (vreg4) { goto block6; } else { goto block14; }
block6: /* empty */;
  signed int vreg7 = *&acc54;
  signed int vreg8 = *&acc54;
  signed int vreg9 = *&j74;
  signed int vreg10 = vreg8 * vreg9;
  signed int vreg11 = vreg10 ^ (signed int)1;
  *vreg7 = vreg11;
  goto block2;
block14: /* empty */;
  signed int vreg15 = *&acc54;
  return vreg15;
}
