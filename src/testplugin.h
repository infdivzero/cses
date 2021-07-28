#ifndef TEST_PLUGIN
#define TEST_PLUGIN

#include <stdint.h>
#include <stddef.h>

/* estdlib.h */

typedef struct { //this must be compatible with the core
    size_t len;
    uint8_t *data;
} page_t;

typedef struct {
    size_t type,    //interface type. Must be equal in both interfaces in a link definition for linkage to occur
           linkmax, //maximum number of interfaces allowed in one link definition entry
           linkidx; //optional, not used by core but can be useful for plugins
} iface_rule;

/* test plugin */

//interface rules - the variable name is the interface name. Indexed and accessed using dlsym
const iface_rule ifaceA0 = {0, 0, 1};
const iface_rule ifaceA1 = {1, 0, 1};
const iface_rule ifaceB0 = {2, 1, 1};
const iface_rule ifaceB1 = {3, 1, 1};

//functions
void init(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);
void loop(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);
void quit(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);

#endif
