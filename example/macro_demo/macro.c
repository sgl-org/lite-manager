#include <stdio.h>
#include "config.h"

extern void macro_a(void);
extern void macro_b(void);

int main(void)
{
    printf("macro demo test\n");
#if CONFIG_MACRO_A
    macro_a();
#endif

#if CONFIG_MACRO_B
    macro_b();
#endif

    return 0;
}
