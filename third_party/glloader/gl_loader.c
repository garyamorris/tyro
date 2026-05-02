#include "gl_loader.h"
#include <stdio.h>
#include <string.h>

#define X(ret, name, args) PFN_gl##name gl##name = 0;
TYRO_GL_FUNCS
#undef X

int tyro_gl_load(tyro_gl_proc_loader loader) {
  if (!loader) return 0;
  int missing = 0;
  #define X(ret, name, args) \
    gl##name = (PFN_gl##name)loader("gl" #name); \
    if (!gl##name) { fprintf(stderr, "[gl_loader] missing gl%s\n", #name); missing++; }
  TYRO_GL_FUNCS
  #undef X
  return missing == 0;
}
