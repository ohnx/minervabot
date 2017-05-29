#include "common.h"
#include "netabs.h"

void mm_register_command(const char *str, void *data, module_callback *callback) {
    
}

void mm_send_raw(bot_context *bc, const char *data) {
    net_raw(bc, data);
}

void mm_send_raw_va(bot_context *bc, const char *data, ...) {
    va_list ap;
    va_start(ap, data);
    vsnprintf(sbuf, 512, data, ap);
    va_end(ap);
    printf("<< %s\n", sbuf);
    write(bc->conn, sbuf, strlen(sbuf));
    write(bc->conn, "\r\n", 2);
}

module_call_table *gen_call_table() {
    module_call_table *mct = calloc(1, sizeof(module_call_table));
    
    mct->register_command = &mm_register_command;
    mct->send_raw = &mm_send_raw;
    mct->send_raw_va = &mm_send_raw_va;
    
    return mct;
}
