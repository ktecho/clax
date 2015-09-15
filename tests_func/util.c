#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ftw.h>
#include <errno.h>

#include "http-parser/http_parser.h"
#include "popen2.h"
#include "util.h"

int http_message_done = 0;
int util_message_complete_cb(http_parser *p)
{
    http_message_done = 1;

    return 0;
}

int util_parse_http_response(char *buf, size_t len)
{
    http_parser parser;
    http_parser_settings settings = {
      .on_message_complete = util_message_complete_cb
    };

    http_parser_init(&parser, HTTP_RESPONSE);
    int ret = http_parser_execute(&parser, &settings, buf, len);

    return ret == len && http_message_done;
}

int execute(char *command, char *request, char *obuf, size_t olen)
{
    int ret;
    popen2_t ctx;

    ret = popen2(command, &ctx);
    if (ret < 0) {
        fprintf(stderr, "Command failed=%d", ret);
        return -1;
    }

    int offset = 0;
    size_t request_len = strlen(request);
    while ((ret = write(ctx.in, request + offset, request_len - offset)) > 0) {
        offset += ret;
    }

    offset = 0;
    while (1) {
        ret = read(ctx.out, obuf + offset, olen - offset);

        if (ret == 0)
            break;

        if (ret < 0 && errno == EAGAIN) {
            continue;
        }

        offset += ret;
    }
    obuf[offset] = 0;

    if (pclose2(&ctx) != 0) {
        fprintf(stderr, "Command failed\n");
        return -1;
    }

    return offset;
}
