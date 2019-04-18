#include "mu.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

struct MU_DATA
{
    struct MHD_Daemon *mhd_daemon;
    uv_poll_t *poll_handle;
};

struct ROUTE
{
    const char *method;
    const char *path;
    MU_REQUEST_HANDLER handler;
};

struct ROUTE *routes_arr = NULL;

size_t routes_arr_size = 0;

MHD_AccessHandlerCallback access_handler_cb;

MHD_RequestCompletedCallback request_completed_cb;

uv_poll_cb on_poll_cb;

static void register_handler(const char *method, const char *path, MU_REQUEST_HANDLER handler);

void shutdown_handler(uv_signal_t *handle, int sig);

int mu_listen(unsigned int port, void (*listen_cb)())
{
    const union MHD_DaemonInfo *info;
    struct MHD_Daemon *d = NULL;
    uv_poll_t poll_handle;
    uv_signal_t sigint_handle, sigterm_handle;
    struct MU_DATA cd = {NULL, &poll_handle};

    uv_signal_init(uv_default_loop(), &sigint_handle);
    uv_signal_init(uv_default_loop(), &sigterm_handle);

    uv_signal_start(&sigint_handle, shutdown_handler, SIGINT);
    uv_signal_start(&sigterm_handle, shutdown_handler, SIGTERM);

    d = MHD_start_daemon(MHD_USE_EPOLL_LINUX_ONLY | MHD_USE_DEBUG,
                         port,
                         NULL,
                         NULL,
                         access_handler_cb,
                         NULL,
                         MHD_OPTION_NOTIFY_COMPLETED, request_completed_cb, NULL,
                         MHD_OPTION_END);
    if (d == NULL)
        return 1;

    listen_cb();

    cd.mhd_daemon = d;

    sigterm_handle.data = sigint_handle.data = poll_handle.data = &cd;

    info = MHD_get_daemon_info(d, MHD_DAEMON_INFO_EPOLL_FD_LINUX_ONLY);

    uv_poll_init(uv_default_loop(), &poll_handle, info->listen_fd);

    uv_poll_start(&poll_handle, UV_READABLE, on_poll_cb);

    return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void mu_get(const char *path, MU_REQUEST_HANDLER handler)
{
    register_handler("GET", path, handler);
}

void mu_post(const char *path, MU_REQUEST_HANDLER handler)
{
    register_handler("POST", path, handler);
}

void mu_put(const char *path, MU_REQUEST_HANDLER handler)
{
    register_handler("PUT", path, handler);
}

void mu_delete(const char *path, MU_REQUEST_HANDLER handler)
{
    register_handler("DELETE", path, handler);
}

void mu_head(const char *path, MU_REQUEST_HANDLER handler)
{
    register_handler("HEAD", path, handler);
}

static void register_handler(const char *method, const char *path, MU_REQUEST_HANDLER handler)
{
    struct ROUTE new_route = { method, path, handler };

    if (routes_arr == NULL)
    {
        routes_arr_size++;
        routes_arr = calloc(1, sizeof(struct ROUTE));
        if (NULL == routes_arr)
        {
            fprintf(stderr, "calloc error: %s\n", strerror(errno));
            abort();
        }
        memcpy(routes_arr, &new_route, sizeof(struct ROUTE));
    }
    else
    {
        routes_arr_size++;
        routes_arr = realloc(routes_arr, routes_arr_size * sizeof(struct ROUTE));
        if (NULL == routes_arr)
        {
            fprintf(stderr, "realloc error: %s\n", strerror(errno));
            abort();
        }
        memcpy(routes_arr + routes_arr_size - 1, &new_route, sizeof(struct ROUTE));
    }
}

struct request_context
{
    char *post_data;
    int post_data_size;
};

static int access_handler(void *cls,
                          struct MHD_Connection *connection,
                          const char *url,
                          const char *method,
                          const char *version,
                          const char *upload_data,
                          size_t *upload_data_size,
                          void **ptr)
{
    static int request_id = 0;
    MU_RESPONSE user_response = {NULL, 0};
    MU_REQUEST user_request = {url, method, NULL, 0};
    struct request_context *request = NULL;
    struct MHD_Response *response;
    int ret;
    unsigned int status_code = MHD_HTTP_OK;
    int route_handled = false;

    request = *ptr;
    if (NULL == request)
    {
        request = calloc(1, sizeof(struct request_context));
        if (NULL == request)
        {
            fprintf(stderr, "calloc error: %s\n", strerror(errno));
            return MHD_NO;
        }
        *ptr = request;
        return MHD_YES;
    }

    if (*upload_data_size > 0)
    {
        request->post_data = realloc(request->post_data, *upload_data_size);
        if (NULL == request->post_data)
        {
            fprintf(stderr, "realloc error: %s\n", strerror(errno));
            return MHD_NO;
        }

        memcpy(request->post_data + request->post_data_size, upload_data, *upload_data_size);

        request->post_data_size += *upload_data_size;

        *upload_data_size = 0; // The number of bytes NOT processed.
        return MHD_YES;
    }

    user_request.body = request->post_data;
    user_request.body_size = request->post_data_size;

    printf("Received request number %d on URL: %s\n", ++request_id, url);

    for (struct ROUTE *route = routes_arr; route < routes_arr + routes_arr_size; route++)
    {
        if (strcmp(route->path, url) == 0 && strcmp(route->method, method) == 0)
        {
            status_code = route->handler(&user_request, &user_response);
            route_handled = true;
            break;
        }
    }

    if (route_handled == false)
    {
        user_response.body = strdup("404 not found");
        user_response.body_size = strlen(user_response.body);
        status_code = MHD_HTTP_NOT_FOUND;
    }

    *ptr = NULL; /* clear context pointer */
    response = MHD_create_response_from_buffer(user_response.body_size,
                                               (void *)user_response.body,
                                               MHD_RESPMEM_MUST_FREE);
    ret = MHD_queue_response(connection,
                             status_code,
                             response);
    MHD_destroy_response(response);
    return ret;
}

MHD_AccessHandlerCallback access_handler_cb = access_handler;

static void on_poll(uv_poll_t *handle, int status, int events)
{
    if (status == 0 && events & UV_READABLE)
    {
        struct MU_DATA *cd = handle->data;
        MHD_run(cd->mhd_daemon);
    }
}

uv_poll_cb on_poll_cb = on_poll;

void shutdown_handler(uv_signal_t *handle, int sig)
{
    struct MU_DATA *cd = handle->data;
    printf("shutting down with signal %d.\n", sig);

    printf("stopping MHD socket poller.\n");
    uv_poll_stop(cd->poll_handle);

    printf("stopping MHD daemon.\n");
    MHD_stop_daemon(cd->mhd_daemon);
    printf("MHD socket closed.\n");

    printf("closing default loop.\n");
    uv_loop_close(uv_default_loop());

    free(routes_arr);
    printf("exiting\n");

    exit(0);
}

static void req_comp(void *cls,
                     struct MHD_Connection *connection,
                     void **con_cls,
                     enum MHD_RequestTerminationCode toe)
{
    struct request_context *request = *con_cls;

    if (NULL == request)
        return;

    free(request->post_data);
    free(request);
}

MHD_RequestCompletedCallback request_completed_cb = req_comp;