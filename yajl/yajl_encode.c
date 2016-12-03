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
    char hexBuf[7];
    hexBuf[0] = '\\'; hexBuf[1] = 'u'; hexBuf[2] = '0'; hexBuf[3] = '0';
    hexBuf[6] = 0;

    while (end < len) {
        if (end < len - 2 && str[end] == '~' && (str[end + 1] == '0' || str[end + 1] == '1')) {
            char c1, c2;
            const char * escaped = NULL;
            if ((c1 = str[end + 1]) == '0' && (c2 = str[end + 2]) == '8')
                 escaped = "\\b";
            else if (c1 == '0' && (c2 == 'c' || c2 == 'C'))
                 escaped = "\\f";
            else if (c1 == '0' && (c2 == 'a' || c2 == 'A'))
                 escaped = "\\n";
            else if (c1 == '0' && (c2 == 'd' || c2 == 'D'))
                 escaped = "\\r";
            else {
                hexBuf[4] = str[end + 1];
                hexBuf[5] = str[end + 2];
                escaped = hexBuf;
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

static void charToHex(unsigned char c, char * hexBuf)
{
    const char * hexchar = "0123456789ABCDEF";
    hexBuf[0] = hexchar[c >> 4];
    hexBuf[1] = hexchar[c & 0x0F];
}

static void hexToDigit(unsigned int * val, const unsigned char * hex)
{
    unsigned int i;
    for (i=0;i<4;i++) {
        unsigned char c = hex[i];
        if (c >= 'A') c = (c & ~0x20) - 7;
        c -= '0';
        assert(!(c & 0xF0));
        *val = (*val << 4) | c;
    }
}

static void Utf32toUtf8(unsigned int codepoint, char * utf8Buf)
{
    if (codepoint < 0x80) {
        utf8Buf[0] = '~';
        charToHex((unsigned char) codepoint, utf8Buf + 1);
        utf8Buf[3] = 0;
    } else if (codepoint < 0x0800) {
        utf8Buf[0] = '~';
        charToHex((unsigned char) ((codepoint >> 6) | 0xC0), utf8Buf + 1);
        utf8Buf[3] = '~';
        charToHex((unsigned char) ((codepoint & 0x3F) | 0x80), utf8Buf + 4);
        utf8Buf[6] = 0;
    } else if (codepoint < 0x10000) {
        utf8Buf[0] = '~';
        charToHex((unsigned char) ((codepoint >> 12) | 0xE0), utf8Buf + 1);
        utf8Buf[3] = '~';
        charToHex((unsigned char) (((codepoint >> 6) & 0x3F) | 0x80), utf8Buf + 4);
        utf8Buf[6] = '~';
        charToHex((unsigned char) ((codepoint & 0x3F) | 0x80), utf8Buf + 7);
        utf8Buf[9] = 0;
    } else if (codepoint < 0x200000) {
        utf8Buf[0] = '~';
        charToHex((unsigned char) ((codepoint >> 18) | 0xF0), utf8Buf + 1);
        utf8Buf[3] = '~';
        charToHex((unsigned char) (((codepoint >> 12) & 0x3F) | 0x80), utf8Buf + 4);
        utf8Buf[6] = '~';
        charToHex((unsigned char) (((codepoint >> 6) & 0x3F) | 0x80), utf8Buf + 7);
        utf8Buf[9] = '~';
        charToHex((unsigned char) ((codepoint & 0x3F) | 0x80), utf8Buf + 10);
        utf8Buf[12] = 0;
    } else {
        utf8Buf[0] = '?';
        utf8Buf[1] = 0;
    }
}

void yajl_string_decode(yajl_buf buf, const unsigned char * str,
                        unsigned int len)
{
    unsigned int beg = 0;
    unsigned int end = 0;

    while (end < len) {
        if (str[end] == '\\') {
            char utf8Buf[13];
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
                case 'u': {
                    unsigned int codepoint = 0;
                    hexToDigit(&codepoint, str + ++end);
                    end+=3;
                    /* check if this is a surrogate */
                    if ((codepoint & 0xFC00) == 0xD800) {
                        end++;
                        if (str[end] == '\\' && str[end + 1] == 'u') {
                            unsigned int surrogate = 0;
                            hexToDigit(&surrogate, str + end + 2);
                            codepoint =
                                (((codepoint & 0x3F) << 10) |
                                 ((((codepoint >> 6) & 0xF) + 1) << 16) |
                                 (surrogate & 0x3FF));
                            end += 5;
                        } else {
                            unescaped = "?";
                            break;
                        }
                    }

                    Utf32toUtf8(codepoint, utf8Buf);
                    unescaped = utf8Buf;
                    break;
                }
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
