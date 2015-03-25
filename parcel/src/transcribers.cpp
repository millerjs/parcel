/******************************************************************************
 * parcel.cpp
 *
 * Parcel udt proxy server
 *
 *****************************************************************************/
#include "parcel.h"

void *udt2pipe(void *_args_)
{
    /*
     *  udt2pipe() - Read from a UDT socket into a pipe
     *
     */
    udt_pipe_args_t *args = (udt_pipe_args_t*) _args_;
    char *buffer = (char*) malloc(BUFF_SIZE);
    int read_size;

    while (1){
        /* Read from UDT */
        read_size = UDT::recv(args->udt_socket, buffer, BUFF_SIZE, 0);
        if (UDT::ERROR == read_size) {
            if (UDT::getlasterror().getErrorCode() != 2001){
                error("recv: %s", UDT::getlasterror().getErrorMessage());
            }
            goto cleanup;
        }
        debug("Read %d bytes from UDT", read_size);

        /* Write to pipe */
        debug("Writing %d bytes to pipe", read_size);
        if (write(args->pipe, buffer, read_size) <= 0){
            error(": Failed to write to pipe.");
            goto cleanup;
        }
        debug("Writing %d bytes to pipe", read_size);
    }

 cleanup:
    debug("Exiting udt2pipe thread.");
    close(args->pipe);
    return NULL;
}

void *tcp2pipe(void *_args_)
{
    /*
     *  tcp2pipe() - Read from a TCP socket into a pipe
     *
     */
    tcp_pipe_args_t *args = (tcp_pipe_args_t*) _args_;
    char *buffer = (char*) malloc(BUFF_SIZE);
    int read_size;

    while (1){
        /* Read from TCP */
        if ((read_size = read(args->tcp_socket, buffer, BUFF_SIZE)) <= 0){
            debug("Unable to read from TCP socket.");
            goto cleanup;
        }
        debug("Read %d bytes from TCP socket %d", read_size, args->tcp_socket);

        /* Write to pipe */
        if (write(args->pipe, buffer, read_size) <= 0){
            error("Failed to write to pipe");
            goto cleanup;
        }
        debug("Wrote %d bytes to pipe", read_size);
    }

 cleanup:
    debug("Exiting tcp2pipe thread.");
    close(args->pipe);
    return NULL;
}

void *pipe2udt(void *_args_)
{
    /*
     *  pipe2udt() - Read from a pipe into a UDT socket
     *
     */
    udt_pipe_args_t *args = (udt_pipe_args_t*) _args_;
    char *buffer = (char*) malloc(BUFF_SIZE);
    int read_size;
    int temp_size;
    int this_size;

    while (1){
        /* Read from pipe */
        if ((read_size = read(args->pipe, buffer, BUFF_SIZE)) <= 0){
            debug("Unable to read from pipe.");
            goto cleanup;
        }
        debug("Read %d bytes from pipe", read_size);

        /* Write to UDT */
        int sent_size = 0;
        debug("Writing %d bytes to UDT socket %d", read_size, args->udt_socket);
        while (sent_size < read_size) {
            this_size = read_size - sent_size;
            temp_size = UDT::send(args->udt_socket, buffer + sent_size, this_size, 0);
            if (UDT::ERROR == temp_size){
                error("send: %s", UDT::getlasterror().getErrorMessage());
                goto cleanup;
            }
            sent_size += temp_size;
        }
        debug("Wrote %d bytes to UDT", read_size);
    }

 cleanup:
    debug("Exiting pipe2udt thread.");
    UDT::close(args->udt_socket);
    close(args->pipe);
    return NULL;
}

void *pipe2tcp(void *_args_)
{
    /*
     *  pipe2tcp() - Read from a pipe into a UDT socket
     *
     */
    tcp_pipe_args_t *args = (tcp_pipe_args_t*) _args_;
    char *buffer = (char*) malloc(BUFF_SIZE);
    int read_size;
    int temp_size;
    int this_size;

    while (1){
        /* Read from pipe */
        if ((read_size = read(args->pipe, buffer, BUFF_SIZE)) <= 0){
            debug("Unable to read from pipe.");
            goto cleanup;
        }
        debug("Read %d bytes from pipe", read_size);

        /* Write to UDT */
        int sent_size = 0;
        debug("Writing %d bytes to TCP socket %d", read_size, args->tcp_socket);
        while (sent_size < read_size) {
            this_size = read_size - sent_size;
            temp_size = send(args->tcp_socket, buffer + sent_size, this_size, 0);
            if (temp_size < 0){
                error("unable to write to socket:");
                goto cleanup;
            }
            sent_size += temp_size;
        }
        debug("Wrote %d bytes to TCP", read_size);
    }

 cleanup:
    debug("Exiting pipe2tcp thread.");
    close(args->tcp_socket);
    close(args->pipe);
    return NULL;
}
