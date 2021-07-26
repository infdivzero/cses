#ifndef TEST_PLUGIN
#define TEST_PLUGIN

#include <stdint.h>
#include <stddef.h>

typedef struct {
    size_t len;
    uint8_t *data;
} page_t;

//define interfaces and their rules here
//might use json for interface rules

void init(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);
void loop(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);
void quit(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);

#endif
