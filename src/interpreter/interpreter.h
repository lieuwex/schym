#pragma once

#include "../intern.h"
#include "./internal.h"

InterEnv* in_make(void);
void in_destroy(InterEnv *is);

RunResult in_run(InterEnv*, const InternedNode);
