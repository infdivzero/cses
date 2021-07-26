#include "testplugin.h"

#include <stdlib.h>
#include <stdio.h>

void init(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec)
{
    printf("init\n");
}

void loop(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec)
{
    printf("loop\n");
    for(size_t i = 0; i < pagec; i++)
    {
        printf("%i\n", pages[i].len);
        for(size_t j = 0; j < pages[i].len; j++)
        {
            putc(pages[i].data[j], stdout);
            if(j > 1024) break;
        }
    }
}

void quit(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec)
{
    printf("quit\n");
}
