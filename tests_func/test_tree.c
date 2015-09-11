#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h> /* stat */

#include "clax_util.h"
#include "u.h"
#include "util.h"

TEST_START(test_tree_upload)
{
    char output[1024];
    char request[1024];
    char body[] = "--------------------------0e6bb0a28f620f98\r\n"
                  "Content-Disposition: form-data; name=\"file\"; filename=\"foobar\"\r\n"
                  "\r\n"
                  "hello from file!\r\n"
                  "--------------------------0e6bb0a28f620f98--\r\n";

    sprintf(request,
            "POST /tree/ HTTP/1.1\r\n"
            "Content-Type: multipart/form-data; boundary=------------------------0e6bb0a28f620f98\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s", (int)strlen(body), body);

    char *tmpdir = mktmpdir();

    char command[1024];
    sprintf(command, "../clax -n -r '%s' -l /dev/null", tmpdir);

    int rcount = execute(command, request, output, sizeof(output));

    ASSERT(rcount > 0);
    ASSERT(util_parse_http_response(output, rcount))

    struct stat st;
    char *fpath = clax_strjoin("/", tmpdir, "foobar", NULL);
    ASSERT(stat(fpath, &st) == 0 && (st.st_mode & S_IFREG));

    free(fpath);
    rmrf(tmpdir);
    free(tmpdir);
}
TEST_END

TEST_START(test_tree_upload_with_different_name)
{
    char output[1024];
    char request[1024];
    char body[] = "--------------------------0e6bb0a28f620f98\r\n"
                  "Content-Disposition: form-data; name=\"file\"; filename=\"foobar\"\r\n"
                  "\r\n"
                  "hello from file!\r\n"
                  "--------------------------0e6bb0a28f620f98--\r\n";

    sprintf(request,
            "POST /tree/?name=another-name HTTP/1.1\r\n"
            "Content-Type: multipart/form-data; boundary=------------------------0e6bb0a28f620f98\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s", (int)strlen(body), body);

    char *tmpdir = mktmpdir();

    char command[1024];
    sprintf(command, "../clax -n -r '%s' -l /dev/null", tmpdir);

    int rcount = execute(command, request, output, sizeof(output));

    ASSERT(rcount > 0);
    ASSERT(util_parse_http_response(output, rcount))

    ASSERT(strstr(output, "200 OK"))

    struct stat st;
    char *fpath = clax_strjoin("/", tmpdir, "another-name", NULL);
    ASSERT(stat(fpath, &st) == 0 && (st.st_mode & S_IFREG));

    free(fpath);
    rmrf(tmpdir);
    free(tmpdir);
}
TEST_END

TEST_START(test_tree_download)
{
    char output[1024];

    int rcount = execute("../clax -n -r . -l /dev/null", "GET /tree/main.c\r\n\r\n", output, sizeof(output));

    ASSERT(rcount > 0);
    ASSERT(util_parse_http_response(output, rcount))

    ASSERT(strstr(output, "200 OK"))
    ASSERT(strstr(output, "Content-Disposition: attachment; filename=\"main.c\""))
    ASSERT(strstr(output, "Last-Modified: "))
}
TEST_END

TEST_START(test_tree_download_not_found)
{
    char output[1024];

    int rcount = execute("../clax -n -r . -l /dev/null", "GET /tree/unlikely-to-exist\r\n\r\n", output, sizeof(output));

    ASSERT(rcount > 0);
    ASSERT(util_parse_http_response(output, rcount))

    ASSERT(strstr(output, "404 Not Found"))
}
TEST_END