/**
 * Autogenerated by the Cthulhu Compiler Collection C99 backend
 * Generated from tests/ctu/pass/funcs/single.ct
 */

// String literals
// Imported symbols

// Global forwarding

// Global initialization

// Function forwarding
void _Z3twov();
void _Z5threev();
void _Z3onev();
void _Z4fourv();

// Function definitions
void _Z3twov() {
  (*&_Z5threev)();
  return;
  return;
}
void _Z5threev() {
  (*&_Z4fourv)();
  return;
  return;
}
void _Z3onev() {
  (*&_Z3twov)();
  return;
  return;
}
void _Z4fourv() {
  (*&_Z3onev)();
  return;
  return;
}
