#ifndef __MINERVA_MODULE_INC
#define __MINERVA_MODULE_INC
#include "common.h"

/* const char *string, void *data */
typedef void (*module_callback)(const char *messsage, const char *channel, bot_context *bc, module_call_table *mct, void *data);

typedef struct _module_call_table {
    void (*register_command)(const char *str, void *data, module_callback *callback);
    void (*send_raw)(bot_context *bc, const char *data);
    void (*send_raw_va)(bot_context *bc, const char *data, ...);
} module_call_table;

typedef struct _module_info_table {
    void (*module_load)(module_call_table *mct);
    void (*module_unload)(module_call_table *mct);
} module_info_table;

#endif


