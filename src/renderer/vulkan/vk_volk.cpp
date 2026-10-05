/*
===========================================================================
  VGLX https://vglx.org
  Copyright © 2024 - Present, Shlomi Nissan
===========================================================================
*/

// volk is compiled as C++ so that VOLK_NAMESPACE can place its function
// pointers in a namespace which keeps them from colliding with a Vulkan
// implementation linked directly into the binary.
#define VOLK_IMPLEMENTATION
#include <volk.h>
