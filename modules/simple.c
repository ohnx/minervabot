#include "common.h"
#include "module.h"
#include <stdio.h>

module_call_table *calls;

void module_callback(const char *message, const char *channel, bot_context *bc, module_call_table *mct, void *data) {
    printf("Called! %s, %p, %p", message, bc, data);
    calls->raw_va("PRIVMSG %s :The test worked! %s", channel, message);
}

void simplemod_load(module_call_table *mct) {
    printf("Loaded module");
    calls = mct;
    mct->register_command("simple", NULL, &module_callback);
}

void simplemod_unload(module_call_table *mct) {
    printf("Unloaded module");
}

module_info_table module = {
    &simplemod_load,
    &simplemod_unload
};
