/*
 * Copyright 2010, Lloyd Hilaiel.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 * 
 *  3. Neither the name of Lloyd Hilaiel nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

#include "yajl_encode.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void
yajl_string_encode(yajl_buf buf, const unsigned char * str,
                   unsigned int len)
{
    yajl_string_encode2((const yajl_print_t) &yajl_buf_append, buf, str, len);
}

void
yajl_string_encode2(const yajl_print_t print,
                    void * ctx,
                    const unsigned char * str,
                    unsigned int len)
{
    unsigned int beg = 0;
    unsigned int end = 0;

    while (end < len) {
        if (end < len - 2 && str[end] == '~' && str[end + 1] == '0') {
            const char * escaped = NULL;
            switch (str[end + 2]) {
                case '8': escaped = "\\b"; break;
                case 'c': case 'C': escaped = "\\f"; break;
                case 'a': case 'A': escaped = "\\n"; break;
                case 'd': case 'D': escaped = "\\r"; break;
                default:
                    break;
            }
            if (escaped != NULL) {
                print(ctx, (const char *) (str + beg), end - beg);
                print(ctx, escaped, (unsigned int)strlen(escaped));
                beg = (end += 3);
            } else {
                ++end;
            }
        } else if (str[end] == '\t') {
            const char * escaped = "\\t";
            print(ctx, (const char *) (str + beg), end - beg);
            print(ctx, escaped, (unsigned int)strlen(escaped));
            beg = ++end;
        } else if (str[end] == '"') {
            const char * escaped = "\\\"";
            print(ctx, (const char *) (str + beg), end - beg);
            print(ctx, escaped, (unsigned int)strlen(escaped));
            beg = ++end;
        } else if (str[end] == '\\') {
            const char * escaped = "\\\\";
            print(ctx, (const char *) (str + beg), end - beg);
            print(ctx, escaped, (unsigned int)strlen(escaped));
            beg = ++end;
        } else {
            ++end;
        }
    }
    print(ctx, (const char *) (str + beg), end - beg);
}

void yajl_string_decode(yajl_buf buf, const unsigned char * str,
                        unsigned int len)
{
    unsigned int beg = 0;
    unsigned int end = 0;

    while (end < len) {
        if (str[end] == '\\') {
            const char * unescaped = "?";
            yajl_buf_append(buf, str + beg, end - beg);
            switch (str[++end]) {
                case '"': unescaped = "\""; break;
                case '\\': unescaped = "\\"; break;
                case '/': unescaped = "/"; break;
                case 'b': unescaped = "~08"; break;
                case 'f': unescaped = "~0C"; break;
                case 'n': unescaped = "~0A"; break;
                case 'r': unescaped = "~0D"; break;
                case 't': unescaped = "\t"; break;
                default:
                    assert("this should never happen" == NULL);
            }
            yajl_buf_append(buf, unescaped, (unsigned int)strlen(unescaped));
            beg = ++end;
        } else {
            end++;
        }
    }
    yajl_buf_append(buf, str + beg, end - beg);
}
