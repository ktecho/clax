#include <stdio.h>
#include <string.h>
#include "clax_http.h"
#include "u.h"

void _init_parser_and_request(http_parser *parser, clax_http_request_t *request)
{
    http_parser_init(parser, HTTP_REQUEST);
    memset(request, 0, sizeof(clax_http_request_t));
}

int _parse(http_parser *parser, clax_http_request_t *req, const char *data)
{
    return clax_http_parse(parser, req, data, strlen(data));
}

TEST_START(clax_http_parse_returns_error_when_error)
{
    http_parser parser;
    clax_http_request_t request;

    _init_parser_and_request(&parser, &request);

    int rv = _parse(&parser, &request, "foobarbaz");

    ASSERT_EQ(rv, -1)
}
TEST_END

TEST_START(clax_http_parse_returns_0_when_need_more)
{
    http_parser parser;
    clax_http_request_t request;

    _init_parser_and_request(&parser, &request);

    int rv = _parse(&parser, &request, "GET / HTTP/1.1\r\n");

    ASSERT_EQ(rv, 0)
}
TEST_END

TEST_START(clax_http_parse_returns_ok_chunks)
{
    http_parser parser;
    clax_http_request_t request;

    _init_parser_and_request(&parser, &request);

    int rv = _parse(&parser, &request, "GET / ");
    ASSERT_EQ(request.headers_done, 0);
    ASSERT_EQ(request.message_done, 0);
    ASSERT_EQ(rv, 0)

    rv = _parse(&parser, &request, "HTTP/1.1\r\n");
    ASSERT_EQ(request.headers_done, 0);
    ASSERT_EQ(request.message_done, 0);
    ASSERT_EQ(rv, 0)

    rv = _parse(&parser, &request, "Host: localhost\r\nConnection: close\r\nContent-Length: 5\r\n");
    ASSERT_EQ(request.headers_done, 0);
    ASSERT_EQ(request.message_done, 0);
    ASSERT_EQ(rv, 0)

    rv = _parse(&parser, &request, "\r\n");
    ASSERT_EQ(request.headers_done, 1);
    ASSERT_EQ(request.message_done, 0);
    ASSERT_EQ(rv, 0)

    rv = _parse(&parser, &request, "hello");
    ASSERT_EQ(request.headers_done, 1);
    ASSERT_EQ(request.message_done, 1);
    ASSERT_EQ(request.body_len, 5);
    ASSERT_EQ(rv, 1)
}
TEST_END

TEST_START(clax_http_parse_returns_done_when_100_continue)
{
    http_parser parser;
    clax_http_request_t request;

    _init_parser_and_request(&parser, &request);

    int rv = _parse(&parser, &request, "GET / HTTP/1.1\r\n");
    rv = _parse(&parser, &request, "Host: localhost\r\nConnection: close\r\nContent-Length: 5\r\nExpect: 100-continue\r\n\r\n");

    ASSERT_EQ(rv, 1)
}
TEST_END

TEST_START(clax_http_parse_returns_ok)
{
    http_parser parser;
    clax_http_request_t request;

    _init_parser_and_request(&parser, &request);

    int rv = _parse(&parser, &request, "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    ASSERT_EQ(rv, 1)
}
TEST_END

TEST_START(clax_http_parse_saves_request)
{
    http_parser parser;
    clax_http_request_t request;

    _init_parser_and_request(&parser, &request);

    _parse(&parser, &request, "GET /there?foo=bar HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");

    ASSERT(request.headers_done)
    ASSERT(request.message_done)
    ASSERT(request.method == HTTP_GET)
    ASSERT_STR_EQ(request.url, "/there?foo=bar")
    ASSERT_STR_EQ(request.path_info, "/there")
    ASSERT_EQ(request.headers_num, 2)
    ASSERT_STR_EQ(request.headers[0].key, "Host")
    ASSERT_STR_EQ(request.headers[0].val, "localhost")
    ASSERT_STR_EQ(request.headers[1].key, "Connection")
    ASSERT_STR_EQ(request.headers[1].val, "close")
}
TEST_END

TEST_START(clax_http_parse_saves_body)
{
    http_parser parser;
    clax_http_request_t request;
    char *p;

    _init_parser_and_request(&parser, &request);

    _parse(&parser, &request, "POST / HTTP/1.1\r\n");
    _parse(&parser, &request, "Host: localhost\r\n");
    _parse(&parser, &request, "Content-Length: 32\r\n");
    _parse(&parser, &request, "\r\n");
    _parse(&parser, &request, "&&&&&foo&foo=&&&foo=bar&=bar&&&&");

    ASSERT_EQ(request.body_len, 32)
    ASSERT_STR_EQ(request.body, "&&&&&foo&foo=&&&foo=bar&=bar&&&&");
}
TEST_END

TEST_START(clax_http_parse_parses_form_body)
{
    http_parser parser;
    clax_http_request_t request;
    char *p;

    _init_parser_and_request(&parser, &request);

    _parse(&parser, &request, "POST / HTTP/1.1\r\n");
    _parse(&parser, &request, "Host: localhost\r\n");
    _parse(&parser, &request, "Content-Type: application/x-www-form-urlencoded\r\n");
    _parse(&parser, &request, "Content-Length: 32\r\n");
    _parse(&parser, &request, "\r\n");
    _parse(&parser, &request, "&&&&&foo&foo=&&&foo=bar&=bar&&&&");

    ASSERT_EQ(request.params_num, 4)
    ASSERT_STR_EQ(request.params[0].key, "foo")
    ASSERT_STR_EQ(request.params[0].val, "")
    ASSERT_STR_EQ(request.params[1].key, "foo")
    ASSERT_STR_EQ(request.params[1].val, "")
    ASSERT_STR_EQ(request.params[2].key, "foo")
    ASSERT_STR_EQ(request.params[2].val, "bar")
    ASSERT_STR_EQ(request.params[3].key, "")
    ASSERT_STR_EQ(request.params[3].val, "bar")
}
TEST_END

TEST_START(clax_http_parse_parses_form_body_with_decoding)
{
    http_parser parser;
    clax_http_request_t request;
    char *p;

    _init_parser_and_request(&parser, &request);

    _parse(&parser, &request, "POST / HTTP/1.1\r\n");
    _parse(&parser, &request, "Host: localhost\r\n");
    _parse(&parser, &request, "Content-Type: application/x-www-form-urlencoded\r\n");
    _parse(&parser, &request, "Content-Length: 16\r\n");
    _parse(&parser, &request, "\r\n");
    _parse(&parser, &request, "f%20o=b%2Fr+baz%");

    ASSERT_EQ(request.params_num, 1)
    ASSERT_STR_EQ(request.params[0].key, "f o")
    ASSERT_STR_EQ(request.params[0].val, "b/r baz%")
}
TEST_END

TEST_START(clax_http_parse_parses_multipart_body)
{
    http_parser parser;
    clax_http_request_t request;
    char *p;

    _init_parser_and_request(&parser, &request);

    _parse(&parser, &request, "POST / HTTP/1.1\r\n");
    _parse(&parser, &request, "Host: localhost\r\n");
    _parse(&parser, &request, "Content-Type: multipart/form-data; boundary=------------------------7ca8ddb13928aa86\r\n");
    _parse(&parser, &request, "Content-Length: 482\r\n");
    _parse(&parser, &request, "\r\n");
    _parse(&parser, &request, "--------------------------7ca8ddb13928aa86\r\n");
    _parse(&parser, &request, "Content-Disposition: form-data; name=\"datafile1\"; filename=\"r.gif\"\r\n");
    _parse(&parser, &request, "Content-Type: image/gif\r\n");
    _parse(&parser, &request, "\r\n");
    _parse(&parser, &request, "foo");
    _parse(&parser, &request, "bar\r\n");
    _parse(&parser, &request, "--------------------------7ca8ddb13928aa86\r\n");
    _parse(&parser, &request, "Content-Disposition: form-data; name=\"datafile2\"; filename=\"g.gif\"\r\n");
    _parse(&parser, &request, "Content-Type: image/gif\r\n");
    _parse(&parser, &request, "\r\n");
    _parse(&parser, &request, "bar");
    _parse(&parser, &request, "baz\r\n");
    _parse(&parser, &request, "--------------------------7ca8ddb13928aa86\r\n");
    _parse(&parser, &request, "Content-Disposition: form-data; name=\"datafile3\"; filename=\"b.gif\"\r\n");
    _parse(&parser, &request, "Content-Type: image/gif\r\n");
    _parse(&parser, &request, "\r\n");
    _parse(&parser, &request, "end\r\n");
    _parse(&parser, &request, "--------------------------7ca8ddb13928aa86--\r\n");

    ASSERT_EQ(request.message_done, 1);

    ASSERT_STR_EQ(request.multipart_boundary, "--------------------------7ca8ddb13928aa86");
    ASSERT_EQ(request.multiparts_num, 3);
    ASSERT_EQ(request.multiparts[0].headers_num, 2);

    ASSERT_STR_EQ(request.multiparts[0].headers[0].key, "Content-Disposition");
    ASSERT_STR_EQ(request.multiparts[0].headers[0].val, "form-data; name=\"datafile1\"; filename=\"r.gif\"");
    ASSERT_STR_EQ(request.multiparts[0].headers[1].key, "Content-Type");
    ASSERT_STR_EQ(request.multiparts[0].headers[1].val, "image/gif");
    ASSERT_EQ(request.multiparts[0].part_len, 6);
    ASSERT_STR_EQ(request.multiparts[0].part, "foobar");

    ASSERT_STR_EQ(request.multiparts[1].headers[0].key, "Content-Disposition");
    ASSERT_STR_EQ(request.multiparts[1].headers[0].val, "form-data; name=\"datafile2\"; filename=\"g.gif\"");
    ASSERT_STR_EQ(request.multiparts[1].headers[1].key, "Content-Type");
    ASSERT_STR_EQ(request.multiparts[1].headers[1].val, "image/gif");
    ASSERT_EQ(request.multiparts[1].part_len, 6);
    ASSERT_STR_EQ(request.multiparts[1].part, "barbaz");

    ASSERT_STR_EQ(request.multiparts[2].headers[0].key, "Content-Disposition");
    ASSERT_STR_EQ(request.multiparts[2].headers[0].val, "form-data; name=\"datafile3\"; filename=\"b.gif\"");
    ASSERT_STR_EQ(request.multiparts[2].headers[1].key, "Content-Type");
    ASSERT_STR_EQ(request.multiparts[2].headers[1].val, "image/gif");
    ASSERT_EQ(request.multiparts[2].part_len, 3);
    ASSERT_STR_EQ(request.multiparts[2].part, "end");
}
TEST_END

TEST_START(clax_http_url_decode_decodes_string_inplace)
{
    char buf[1024];

    strcpy(buf, "hello");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "hello")

    strcpy(buf, "f%20o");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "f o")

    strcpy(buf, "f+o");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "f o")

    strcpy(buf, "fo%");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "fo%")

    strcpy(buf, "fo%2");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "fo%2")

    strcpy(buf, "fo%20");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "fo ")

    strcpy(buf, "fo%20%20");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "fo  ")

    strcpy(buf, "fo%20%20bar");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "fo  bar")

    strcpy(buf, "while%20true%3B%20do%20echo%20foo%3B%20done");
    clax_http_url_decode(buf);
    ASSERT_STR_EQ(buf, "while true; do echo foo; done")
}
TEST_END

TEST_START(clax_http_status_message_returns_message)
{
    ASSERT_STR_EQ(clax_http_status_message(200), "OK")
}
TEST_END

TEST_START(clax_http_status_message_returns_unknown_message)
{
    ASSERT_STR_EQ(clax_http_status_message(999), "Unknown")
}
TEST_END

TEST_START(clax_http_extract_kv_extracts_val)
{
    const char *val;
    size_t len;

    val = clax_http_extract_kv("name=\"upload\"; filename=\"file.zip\"", "name", &len);
    ASSERT_STRN_EQ(val, "upload", len);
    ASSERT_EQ(len, 6);

    val = clax_http_extract_kv("name=\"upload\"; filename=\"file.zip\"", "filename", &len);
    ASSERT_STRN_EQ(val, "file.zip", len);
    ASSERT_EQ(len, 8);

    val = clax_http_extract_kv("name=\"upload\"; filename=\"file.zip\"", "something", &len);
    ASSERT(val == NULL);
    ASSERT_EQ(len, 0);
}
TEST_END
