# libmu
__libmu__ (Manticore Unicorn Library) is a library for creating simple, asynchronous HTTP servers in pure C based on [GNU Libmicrohttpd](https://www.gnu.org/software/libmicrohttpd/) and [libuv](http://libuv.org/).

## Example
```
#include "mu.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const int PORT = 3000;

unsigned int say_hello(const MU_REQUEST *req, MU_RESPONSE *res);

unsigned int echo(const MU_REQUEST *req, MU_RESPONSE *res);

void listen_cb();

int main(void)
{
    mu_get("/", say_hello);

    mu_post("/", echo);

    return mu_listen(PORT, listen_cb);
}

unsigned int say_hello(const MU_REQUEST *req, MU_RESPONSE *res)
{
    res->body = strdup("Hello World!");
    res->body_size = strlen(res->body);
    return 200;
}

unsigned int echo(const MU_REQUEST *req, MU_RESPONSE *res)
{
    res->body = malloc(req->body_size + 2);
    sprintf(res->body, "%.*s\n", req->body_size, req->body);
    res->body_size = strlen(res->body);
    return 200;
}

void listen_cb()
{
    printf("libmu server listening on port %d\n", PORT);
}
```
