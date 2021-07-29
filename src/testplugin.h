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
    uint8_t type[16]; //interface type. Must be equal in both interfaces in a link definition for linkage to occur. UUID
    size_t  linkmax;  //maximum number of interfaces of this type allowed in one link definition entry
} iface_rule;

/* test plugin */

//interface rules - the variable name is the interface name. Indexed and accessed using dlsym
//^^^ method currently provides redundant information. Suggest replacing iface_rule with a string which
//    references the rule (renamed to type) to use
const iface_rule ifaceA0 = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 2};
const iface_rule ifaceA1 = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 2};
const iface_rule ifaceB0 = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 2};
const iface_rule ifaceB1 = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 2};

//functions
void init(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);
void loop(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);
void quit(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);

#endif
