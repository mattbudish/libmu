#ifndef _MU_H_
#define _mu_H_

#include <microhttpd.h>
#include <uv.h>

typedef struct mu_request
{
    const char *url;
    const char *method;
    const char *body;
    unsigned int body_size;
} MU_REQUEST;

typedef struct mu_response
{
    char *body;
    unsigned int body_size;
} MU_RESPONSE;

typedef unsigned int (*MU_REQUEST_HANDLER)(
    const MU_REQUEST *req,
    MU_RESPONSE *res);

void mu_get(const char *path, MU_REQUEST_HANDLER handler);

int mu_listen(unsigned int port, void (*listen_cb)());

#endif