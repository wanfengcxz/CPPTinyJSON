//
// Created by 晚风吹行舟 on 2023/5/8.
//

#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>

#endif

#include <cassert>
#include <cstring>
#include <cstdlib>

#include "tiny_json.h"

namespace tiny_json {

    struct Context {
        const char *json;
        char *stack;
        size_t size, top;
    };


    // 所谓空白，是由零或多个空格符（space U+0020）、
    // 制表符（tab U+0009）、换行符（LF U+000A）、回车符（CR U+000D）所组成。
    // ws = *(%x20 / %x09 / %x0A / %x0D)
    static void parse_whitespace(Context &c) {
        const char *p = c.json;
        while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
            p++;
        c.json = p;
    }

    // null = "null"
//    static int parse_null(Context &c, Value &v) {
//        assert(*c.json == 'n');
//        ++c.json;
//        if (c.json[0] == 'u' && c.json[1] == 'l' && c.json[2] == 'l') {
//            c.json += 3;
//            v.type = NUL;
//            return PARSE_OK;
//        } else {
//            return PARSE_INVALID_VALUE;
//        }
//    }


    static int parse_literal(Context &c, Value &v, const char *literal, Type type) {
        while (*literal != '\0') {
            if (*c.json != *literal) return PARSE_INVALID_VALUE;
            ++literal, ++c.json;
        }
        v.type = type;
        return PARSE_OK;
    }


    // value = true
//    static int parse_false(Context &c, Value &v) {
//        assert(*c.json == 'f');
//        ++c.json;
//        if (c.json[0] == 'a' && c.json[1] == 'l' && c.json[2] == 's' && c.json[3] == 'arr') {
//            c.json += 4;
//            v.type = FALSE;
//            return PARSE_OK;
//        } else {
//            return PARSE_INVALID_VALUE;
//        }
//    }

    // value = true
//    static int parse_true(Context &c, Value &v) {
//        assert(*c.json == 't');
//        ++c.json;
//        if (c.json[0] == 'r' && c.json[1] == 'u' && c.json[2] == 'arr') {
//            c.json += 3;
//            v.type = TRUE;
//            return PARSE_OK;
//        } else {
//            return PARSE_INVALID_VALUE;
//        }
//    }

#define IS_DIGIT_1_9(ch) (ch <= '9' && ch > '0')
#define IS_DIGIT(ch) (ch <= '9' && ch >= '0')

    static int parse_number(Context &c, Value &v) {

        const char *p = c.json;
        if (*p == '-') ++p;
        if (*p == '0') {
            ++p;
            if (IS_DIGIT(*p)) return PARSE_INVALID_VALUE;
        } else {
            if (!IS_DIGIT_1_9(*p)) return PARSE_INVALID_VALUE;
            while (IS_DIGIT_1_9(*p)) ++p;
        }
        if (*p == '.') {
            p++;
            if (!IS_DIGIT(*p)) return PARSE_INVALID_VALUE;
            while (IS_DIGIT(*p)) ++p;
        }
        if (*p == 'E' || *p == 'e') {
            p++;
            if (*p == '+' || *p == '-') p++;
            if (!IS_DIGIT(*p)) return PARSE_INVALID_VALUE;
            while (IS_DIGIT(*p)) ++p;
        }
//        if (*p != '\0') return PARSE_INVALID_VALUE;

        v.num = strtod(c.json, NULL);
        c.json = p;
        v.type = NUMBER;
        return PARSE_OK;
    }

    void value_free(Value &v) {
//        if (v.type == STRING)
//            free(v.str);
//          这种写法嵌套的对象无法释放
//        if (v.type == ARRAY)
//            free(v.arr);
        switch (v.type) {
            case STRING:
                free(v.str);
                break;
            case ARRAY:
                for (size_t i = 0; i < v.a_size; i++)
                    value_free(v.arr[i]);
                free(v.arr);
                break;
            case OBJECT:
                for (size_t i = 0; i < v.m_size; i++) {
                    free(v.m[i].k);
                    value_free(v.m[i].v);
                }
                free(v.m);
                break;
            default:
                break;
        }
        v.type = NUL;
    }

#ifndef PARSE_STACK_INIT_SIZE
#define PARSE_STACK_INIT_SIZE 256
#endif

    static void *context_push(Context &c, size_t size) {
        void *ret;
        assert(size > 0);
        if (c.top + size >= c.size) {
            if (c.size == 0)
                c.size = PARSE_STACK_INIT_SIZE;
            while (c.top + size >= c.size) c.size += c.size >> 1;
            c.stack = (char *) realloc(c.stack, c.size);
        }
        ret = c.stack + c.top;
        c.top += size;
        return ret;
    }

    static void *context_pop(Context &c, size_t size) {
        assert(c.top >= size);
        c.top -= size;
        return c.stack + c.top;
    }

    static const char *parse_hex4(const char *p, unsigned int &u) {
        u = 0;
        for (int i = 0; i < 4; i++) {
            char ch = *p++;
            u = u << 4;
            if (ch >= '0' && ch <= '9') u |= ch - '0';
            else if (ch >= 'A' && ch <= 'F') u |= ch - 'A' + 10;
            else if (ch >= 'a' && ch <= 'f') u |= ch - 'a' + 10;
            else return NULL;
        }
        return p;
    }

    static void encode_utf8(Context &c, unsigned u) {
        if (0 <= u && u <= 0x7F) {
            *(char *) context_push(c, 1) = u & 0xFF;
        } else if (u <= 0x7FF) {
            *(char *) context_push(c, 1) = ((u >> 6) & 0xFF) | 0xC0;
            *(char *) context_push(c, 1) = (u & 0x3F) | 0x80;
        } else if (u <= 0xFFFF) {
            *(char *) context_push(c, 1) = ((u >> 12) & 0xF) | 0xE0;
            *(char *) context_push(c, 1) = ((u >> 6) & 0x3F) | 0x80;
            *(char *) context_push(c, 1) = (u & 0x3F) | 0x80;
        } else if (u <= 0x10FFFF) {
            *(char *) context_push(c, 1) = ((u >> 18) & 0x7) | 0xF0;
            *(char *) context_push(c, 1) = ((u >> 12) & 0x3F) | 0x80;
            *(char *) context_push(c, 1) = ((u >> 6) & 0x3F) | 0x80;
            *(char *) context_push(c, 1) = (u & 0x3F) | 0x80;
        }
    }

    static int parse_string_raw(Context &c, size_t &len) {
        size_t start = c.top;
        const char *p;
        assert(*c.json == '"');
        p = ++c.json;
        unsigned int u, u2;
        while (true) {
            char ch = *(p++);
            switch (ch) {
                case '\"':
                    len = c.top - start;
                    c.json = p;
                    return PARSE_OK;
                case '\0':
                    c.top = start;
                    return PARSE_MISS_QUOTATION_MARK;
                case '\\':
                    switch (*p++) {
                        case '\"':
                            *(char *) context_push(c, sizeof(char)) = '\"';
                            break;
                        case '\\':
                            *(char *) context_push(c, sizeof(char)) = '\\';
                            break;
                        case '/':
                            *(char *) context_push(c, sizeof(char)) = '/';
                            break;
                        case 'b':
                            *(char *) context_push(c, sizeof(char)) = '\b';
                            break;
                        case 'f':
                            *(char *) context_push(c, sizeof(char)) = '\f';
                            break;
                        case 'n':
                            *(char *) context_push(c, sizeof(char)) = '\n';
                            break;
                        case 'r':
                            *(char *) context_push(c, sizeof(char)) = '\r';
                            break;
                        case 't':
                            *(char *) context_push(c, sizeof(char)) = '\t';
                            break;
                        case 'u':
                            if (!(p = parse_hex4(p, u))) {
                                c.top = start;
                                return PARSE_INVALID_UNICODE_HEX;
                            }
                            if (u >= 0xD800 && u <= 0xDBFF) {    // surrogate pair
                                if (*p++ != '\\') {
                                    c.top = start;
                                    return PARSE_INVALID_UNICODE_SURROGATE;
                                }
                                if (*p++ != 'u') {
                                    c.top = start;
                                    return PARSE_INVALID_UNICODE_SURROGATE;
                                }
                                if (!(p = parse_hex4(p, u2))) {
                                    c.top = start;
                                    return PARSE_INVALID_UNICODE_SURROGATE;
                                }
                                if (u2 < 0xDC00 || u2 > 0xDFFF) {
                                    c.top = start;
                                    return PARSE_INVALID_UNICODE_SURROGATE;
                                }
                                u = (((u - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
                            }
                            encode_utf8(c, u);
                            break;
                        default:
                            c.top = start;
                            return PARSE_INVALID_STRING_ESCAPE;
                    }
                    break;
                default:
                    if ((unsigned char) ch < 0x20) {
                        c.top = start;
                        return PARSE_INVALID_STRING_CHAR;
                    }
                    char *tmp = (char *) context_push(c, sizeof(char));
                    *tmp = ch;
            }

        }
    }

    static int parse_string(Context &c, Value &v) {
        size_t len = 0;
        int ret = parse_string_raw(c, len);
        if (ret == PARSE_OK)
            set_string(v, (const char *) context_pop(c, len), len);
        return ret;
    }

    static int parse_value(Context &c, Value &v);

    static int parse_array(Context &c, Value &v) {
        assert(*c.json == '[');
        ++c.json;
        parse_whitespace(c);
        if (*c.json == ']') {
            ++c.json;
            v.type = ARRAY;
            v.a_size = 0;
            v.arr = nullptr;
            return PARSE_OK;
        }

        size_t size = 0;
        int ret;
        while (true) {
            Value tmp_v{};
            init(v);

            parse_whitespace(c);
            ret = parse_value(c, tmp_v);
            if (ret != PARSE_OK) {
                break;
//                c.top = 0;
//                return ret;
            }
            memcpy(context_push(c, sizeof(Value)), &tmp_v, sizeof(value));
            size++;

            parse_whitespace(c);
            if (*c.json == ',') c.json++;
            else if (*c.json == ']') {
                ++c.json;
                v.a_size = size;
                v.type = ARRAY;
                size *= sizeof(Value);
                v.arr = (Value *) malloc(size);
                memcpy(v.arr, context_pop(c, size), size);
                return PARSE_OK;
            } else {
                // 如果只是c.top = 0，那么stack中的内存没有被释放掉
                // 这个字符串解析失败的做法一样
                // 这里可以先break，然后在后面释放掉
//                c.top = 0;
//                return PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
                ret = PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
                break;
            }
        }
        for (int i = 0; i < size; i++)
            value_free(*(Value *) context_pop(c, sizeof(Value)));
        return ret;
    }

    static int parse_object(Context &c, Value &v) {
        assert(*c.json == '{');
        ++c.json;
        parse_whitespace(c);
        if (*c.json == '}') {
            ++c.json;
            v.type = OBJECT;
            v.m_size = 0;
            v.m = nullptr;
            return PARSE_OK;
        }

        size_t size = 0;
        member m{};
        int ret = 0;
        while (true) {
            Value tmp;
            init(tmp);
            init(m.v);

            // parse key
            parse_whitespace(c);
            if (*c.json != '"') {
                ret = PARSE_MISS_KEY;
                break;
            }
            ret = parse_string(c, tmp);
            if (ret != PARSE_OK) break;
            assert(tmp.type == STRING);
            m.k_len = tmp.len;
            m.k = tmp.str;

            // parse ws colon ws
            parse_whitespace(c);
            if (*c.json != ':') {
                ret = PARSE_MISS_COLON;
                break;
            }
            ++c.json;
            parse_whitespace(c);

            // parse value
            ret = parse_value(c, m.v);
            if (ret != PARSE_OK) break;
            memcpy(context_push(c, sizeof(member)), &m, sizeof(member));
            size++;
            m.k = nullptr;

            // parse ws [comma | right-curly-brace] ws
            parse_whitespace(c);
            if (*c.json == ',') ++c.json;
            else if (*c.json == '}') {
                ++c.json;
                v.m_size = size;
                v.type = OBJECT;
                size = size * sizeof(member);
                v.m = (member *) malloc(size);
                memcpy(v.m, context_pop(c, size), size);
                return PARSE_OK;
            } else {
                ret = PARSE_MISS_COMMA_OR_CURLY_BRACKET;
                break;
            }
        }
        free(m.k);
        for (int i = 0; i < size; i++) {
            auto *tmp = (member *) context_pop(c, sizeof(member));
            free(tmp->k);
            value_free(tmp->v);
        }
        v.type = NUL;
        return ret;
    }

    static int parse_value(Context &c, Value &v) {
        switch (*(c.json)) {
            case 'n':
                return parse_literal(c, v, "null", NUL);
            case 'f':
                return parse_literal(c, v, "false", FALSE);
            case 't':
                return parse_literal(c, v, "true", TRUE);
            case '"':
                return parse_string(c, v);
            case '[':
                return parse_array(c, v);
            case '{':
                return parse_object(c, v);
            case '\0' :
                return PARSE_EXPECT_VALUE;
            default:
                return parse_number(c, v);
        }
    }

    // JSON-text = ws value ws
    int parse(Value &v, const char *json) {
        Context c;
        c.json = json;
        c.stack = NULL;
        c.size = c.top = 0;
        init(v);
        parse_whitespace(c);
        int ret = parse_value(c, v);
        if (ret == PARSE_OK) {
            parse_whitespace(c);
            if (*c.json != '\0') return PARSE_ROOT_NOT_SINGULAR;
        }
        assert(c.top == 0);
        free(c.stack);
        return ret;
    }


    Type get_type(const Value &v) {
        return v.type;
    }

    int get_boolean(const Value &v) {
        assert(v.type == FALSE || v.type == TRUE);
        return v.type;
    }

    void set_boolean(Value &v, bool flag) {
        value_free(v);
        v.type = flag ? TRUE : FALSE;
    }

    double get_number(const Value &v) {
        assert(v.type == NUMBER);
        return v.num;
    }

    void set_number(Value &v, double num) {
//        value_free(v);
        v.type = NUMBER;
        v.num = num;
    }

    const char *get_string(const Value &v) {
        assert(v.type == STRING);
        return v.str;
    }

    size_t get_string_length(Value &v) {
        assert(v.type == STRING);
        return strlen(v.str);
    }

    void set_string(Value &v, const char *s, size_t len) {
        assert(s != NULL || len == 0);
        value_free(v);
        v.str = (char *) malloc(len + 1);
        memcpy(v.str, s, len);
        v.str[len] = '\0';
        v.len = len;
        v.type = STRING;
    }

    size_t get_array_size(const Value &v) {
        assert(v.type == ARRAY);
        return v.a_size;
    }

    Value *get_array_element(const Value &v, size_t index) {
        assert(v.type == ARRAY);
        assert(v.a_size > index);
        return &v.arr[index];
    }

    size_t get_object_size(const Value &v) {
        assert(v.type == OBJECT);
        return v.m_size;
    }

    const char *get_object_key(const Value &v, size_t index) {
        assert(v.type == OBJECT);
        assert(v.m_size > index);
        return v.m[index].k;
    }

    size_t get_object_key_length(const Value &v, size_t index) {
        assert(v.type == OBJECT);
        assert(v.m_size > index);
        return v.m[index].k_len;
    }

    Value *get_object_value(const Value &v, size_t index) {
        assert(v.type == OBJECT);
        assert(v.m_size > index);
        return &v.m[index].v;
    }

//    char *stringify(const Value &v, size_t &len) {
//
//    }

}























