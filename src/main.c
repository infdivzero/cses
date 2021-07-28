#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "cJSON.h"

//clock frequencies are emulated by the plugins
//^^^ emulation standard library?  (estdlib.h)
//what about multiple systems in one process? Live interaction and management?
//gotta look into daemons and ipc
//also gotta figure out composite links for bandwidth >64 bits
//simd might come in handy for some stuff in this software but emulation code
//  may benefit from it more
//a binary configuration interface might be implemented
//a way to detect duplicated links will be useful
//gonna focus on creating a single vm first before expanding to multiple system
//  emulation, live interaction, and syslinks
//only one component per plugin can be implemented
//should come up with error codes
//per component errors? Can't easily not alloc a component without keeping the handles separate

//use xxd -i for inclusion of static config files

//write the programmable parser before implementing multivm

/* estdlib.h */

typedef struct {
    size_t len;
    uint8_t *data;
} page_t;

typedef struct {
    size_t type,    //interface type. Must be equal in both interfaces in a link definition for linkage to occur
           linkmax, //maximum number of interfaces allowed in one link definition entry
           linkidx; //optional, not used by core but can be useful for plugins
} iface_rule;

/* defs */

typedef void (*pl_func)(uint64_t **link_ptrs, size_t linkc, page_t *pages, size_t pagec);

typedef struct {
    page_t    *pages;
    uint64_t **links;
    size_t pagec;
    size_t linkc;
    char *componentname;
    void *plugin;
    pl_func init;
    pl_func loop;
    pl_func quit;
} component_instance;

typedef struct {
    char *vm,
         *component,
         *interface;
    size_t vm_len,
           component_len,
           interface_len;
} iface_def;

typedef struct {
    iface_def *ifaces;
    size_t     ifacec;
} link_def;

/* functions */

int ldfile(char *path, char **out_data, size_t *out_len)
{
    int err = 0;
    
    FILE *fp = fopen(path, "r");
    
    if(fp)
    {
        fseek(fp, 0L, SEEK_END);
        *out_len = ftell(fp);
        rewind(fp);
        char *tmp = realloc(*out_data, *out_len + 1);
        
        if(tmp)
        {
            *out_data = tmp;
            fread(*out_data, *out_len, 1, fp);
        }
        else err = 1;
        
        fclose(fp);
    }
    else err = 1;
    
    return err; //0 on success
}

char* pathcat(char *dir, char *file) //ret must be freed
{
    char *ret = malloc(strlen(file) + strlen(dir) + 2);
    strcpy(ret, dir);
    strcat(ret, "/");
    strcat(ret, file);
    return ret;
}

static inline int errcond(int cond, unsigned int *err, unsigned int code) //code must be >0 for error
{
    return cond && !*err? 1 : !(*err = code); //fun piece of code
}

int main(int argc, char **argv)
{
    int exec = 0;
    
    unsigned int err = 0;
    
    char *vmname;
    
    component_instance *components = NULL; //components to emulate for the current vm
    uint64_t           *links      = NULL; //shared data for communication between components
    
    size_t componentc = 0, linkc = 0;
    
    components = malloc(0);
    links      = malloc(0);
    
    //open config file from args
    if(argc > 1) //will implement option parsing and iterative config loading for multisystem support
    {
        cJSON *config = NULL;
        
        link_def *linkdefs = malloc(0);
        size_t    linkdefc = 0;
        
        char *fdata = malloc(0);
        size_t flen = 0;
        err = ldfile(argv[1], &fdata, &flen);
        config = cJSON_ParseWithLength(fdata, flen);
        free(fdata);
        
        if(config && !err)
        {
            //would like to write a programmable parser control to make config reading more flexible and less redundant
            cJSON *item;
            cJSON *arr;
            char *plugindir, *datadir;
            int reqcomp = 1;
            
            item = cJSON_GetObjectItem(config, "vmname");
            if(errcond(cJSON_IsString(item), &err, 1)) vmname = cJSON_GetStringValue(item);
            
            item = cJSON_GetObjectItem(config, "plugindir"); //would like to implement multiple plugin dirs
            if(errcond(cJSON_IsString(item), &err, 1)) plugindir = cJSON_GetStringValue(item);
            
            item = cJSON_GetObjectItem(config, "datadir"); //would like to implement multiple data dirs
            if(errcond(cJSON_IsString(item), &err, 1)) datadir = cJSON_GetStringValue(item);
            
            arr = cJSON_GetObjectItem(config, "components");
            if(errcond(cJSON_IsArray(arr), &err, 1))
            {
                for(size_t i = 0; i < cJSON_GetArraySize(arr); i++)
                {
                    cJSON *obj = cJSON_GetArrayItem(arr, i);
                    
                    component_instance component;
                    
                    component.pages = malloc(0);
                    component.pagec = 0;
                    
                    item = cJSON_GetObjectItem(obj, "plugin");
                    if(errcond(cJSON_IsString(item), &err, 1))
                    {
                        char *soname = cJSON_GetStringValue(item);
                        char *path = pathcat(plugindir, soname);
                        component.plugin = dlopen(path, RTLD_LAZY);
                        if(component.plugin)
                        {
                            component.init = dlsym(component.plugin, "init");
                            component.loop = dlsym(component.plugin, "loop");
                            component.quit = dlsym(component.plugin, "quit");
                        }
                        else err = 1;
                        //error code
                        free(path);
                    }
                    
                    //register component ID
                    item = cJSON_GetObjectItem(obj, "componentname");
                    if(errcond(cJSON_IsString(item), &err, 1))
                    {
                        char *str = cJSON_GetStringValue(item);
                        component.componentname = malloc(strlen(str) + 1);
                        strcpy(component.componentname, str);
                    }
                    
                    //load and allocate component instance data
                    cJSON *objarr = cJSON_GetObjectItem(obj, "data");
                    if(errcond(cJSON_IsArray(objarr), &err, 1))
                    {
                        for(size_t j = 0; j < cJSON_GetArraySize(objarr); j++)
                        {
                            //obj is not used again in the i loop
                            obj = cJSON_GetArrayItem(objarr, j);
                            
                            page_t page;
                            char *file;
                            
                            page.len = 0;
                            
                            if(errcond((obj != NULL), &err, 1))
                            {
                                item = cJSON_GetObjectItem(obj, "allocate");
                                if(cJSON_IsNumber(item)) page.len = cJSON_GetNumberValue(item);
                                
                                item = cJSON_GetObjectItem(obj, "file");
                                if(cJSON_IsString(item)) file = cJSON_GetStringValue(item);
                                
                                //allocate pages
                                page.data = calloc(page.len, 1);
                                
                                //load files
                                if(file)
                                {
                                    char *path = pathcat(datadir, file);
                                    char *tmpdata = malloc(0);
                                    size_t flen = 0;
                                    err = ldfile(path, &tmpdata, &flen);
                                    if(!err)
                                    {
                                        if(page.len < flen)
                                        {
                                            if(page.len == 0) free(page.data);
                                            page.len = flen;
                                            page.data = tmpdata;
                                        }
                                        else
                                        {
                                            memcpy(page.data, tmpdata, flen);
                                            free(tmpdata);
                                        }
                                    }
                                    free(path);
                                }
                                
                                file = NULL; //make sure files aren't loaded for entries without them
                                
                                page_t *tmp = realloc(component.pages, ++component.pagec * sizeof(page_t));
                                if(tmp)
                                {
                                    component.pages = tmp;
                                    memcpy(&component.pages[component.pagec - 1], &page, sizeof(page_t));
                                }
                                else
                                {
                                    component.pagec--;
                                    err = 1;
                                    //error code
                                }
                            }
                            
                            if(err) break; //would prefer to have vm not incomplete. Will make configurable per vm
                        }
                    }
                    
                    //add new component to vm component array
                    component_instance *tmp = realloc(components, ++componentc * sizeof(component_instance));
                    if(tmp)
                    {
                        components = tmp;
                        memcpy(&components[componentc - 1], &component, sizeof(component_instance));
                    }
                    else
                    {
                        componentc--;
                        err = 1;
                        //error code
                    }
                    
                    if(err) break;
                    
                    //prevent creation of vm rather than creating an incomplete one
                }
            }
            
            arr = cJSON_GetObjectItem(config, "links");
            if(errcond(cJSON_IsArray(arr), &err, 1))
            {
                for(size_t i = 0; i < cJSON_GetArraySize(arr); i++)
                {
                    //duplicate link detection?
                    cJSON *link = cJSON_GetArrayItem(arr, i);
                    if(errcond(cJSON_IsArray(link), &err, 1))
                    {
                        link_def linkdef;
                        
                        linkdef.ifaces = malloc(0);
                        linkdef.ifacec = 0;
                        
                        //parse linkdef
                        for(size_t j = 0; j < cJSON_GetArraySize(link); j++)
                        {
                            iface_def ifacedef;
                            
                            cJSON *obj = cJSON_GetArrayItem(link, j);
                            cJSON *obji;
                            
                            obji = cJSON_GetObjectItem(obj, "vm");
                            if(errcond(cJSON_IsString(obji), &err, 1))
                            {
                                char *str = cJSON_GetStringValue(obji);
                                ifacedef.vm_len = strlen(str);
                                ifacedef.vm = malloc(++ifacedef.vm_len);
                                strcpy(ifacedef.vm, str);
                            }
                            
                            obji = cJSON_GetObjectItem(obj, "component");
                            if(errcond(cJSON_IsString(obji), &err, 1))
                            {
                                char *tmp = cJSON_GetStringValue(obji);
                                ifacedef.component_len = strlen(tmp);
                                ifacedef.component = malloc(++ifacedef.component_len);
                                strcpy(ifacedef.component, tmp);
                            }
                            
                            obji = cJSON_GetObjectItem(obj, "interface");
                            if(errcond(cJSON_IsString(obji), &err, 1))
                            {
                                char *tmp = cJSON_GetStringValue(obji);
                                ifacedef.interface_len = strlen(tmp);
                                ifacedef.interface = malloc(++ifacedef.interface_len);
                                strcpy(ifacedef.interface, tmp);
                            }
                            
                            //alloc the interface definition for the current link entry
                            iface_def *tmp = realloc(linkdef.ifaces, sizeof(iface_def) * (linkdef.ifacec + 1));
                            if(errcond(tmp != NULL, &err, 1))
                            {
                                linkdef.ifaces = tmp;
                                memcpy(&linkdef.ifaces[linkdef.ifacec], &ifacedef, sizeof(ifacedef));
                                linkdef.ifacec++;
                            }
                        }
                        
                        //alloc the current link entry
                        link_def *tmp = realloc(linkdefs, sizeof(link_def) * (linkdefc + 1));
                        if(errcond(tmp != NULL, &err, 1))
                        {
                            linkdefs = tmp;
                            memcpy(&linkdefs[linkdefc], &linkdef, sizeof(linkdef));
                            linkdefc++;
                        }
                    }
                }
            }
        }
        
        cJSON_Delete(config);
    
        if(!err)
        {
            exec = 1;
            //link the components. This includes components from all systems
            //iterate linkdefs
            //  realloc shared memory
            //  iterate iface defs
            //      check interface rules
            //          dlsym the component's plugin using the ifacedef's interface name to load the rule
            //      locate pointer specified in the iface def and point it to the current shared mem
            //  recycle the current shared memory if an error occured
        }
        else printf("System definition parsing failed with error code %i", err);
        
        for(size_t i = 0; i < linkdefc; i++)
        {
            for(size_t j = 0; j < linkdefs[i].ifacec; j++)
            {
                free(linkdefs[i].ifaces[j].vm);
                free(linkdefs[i].ifaces[j].component);
                free(linkdefs[i].ifaces[j].interface);
            }
            free(linkdefs[i].ifaces);
        }
        free(linkdefs);
    }
    
    if(!err)
    {
        //for(size_t i = 0; i < componentc; i++)
            //if(components[i].init) components[i].init(components[i].links, components[i].linkc, components[i].pages, components[i].pagec);
        
        while(exec)
        {
            //call plugin update functions for each instance according to its timing rules
            //for(size_t i = 0; i < componentc; i++)
                //if(components[i].loop) components[i].loop(components[i].links, components[i].linkc, components[i].pages, components[i].pagec);
            exec = 0;
        }
        
        //for(size_t i = 0; i < componentc; i++)
            //if(components[i].quit) components[i].quit(components[i].links, components[i].linkc, components[i].pages, components[i].pagec);
    }
    
    for(size_t i = 0; i < componentc; i++)
    {
        free(components[i].componentname);
        dlclose(components[i].plugin);
        for(size_t j = 0; j < components[i].pagec; j++) free(components[i].pages[j].data);
        free(components[i].pages);
    }
    
    free(components);
    free(links);
    
    return 0;
}
