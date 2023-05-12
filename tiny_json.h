//
// Created by 晚风吹行舟 on 2023/5/8.
//

#ifndef CPPTINYJSON_TINY_JSON_H
#define CPPTINYJSON_TINY_JSON_H

#ifdef _WINDOWS
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

namespace tiny_json {

    enum Type {
        NUL, FALSE, TRUE, NUMBER, STRING, ARRAY, OBJECT
    };

    enum {
        PARSE_OK = 0,
        PARSE_EXPECT_VALUE,
        PARSE_INVALID_VALUE,
        PARSE_ROOT_NOT_SINGULAR,
        PARSE_MISS_QUOTATION_MARK,
        PARSE_INVALID_STRING_ESCAPE,
        PARSE_INVALID_STRING_CHAR,
        PARSE_INVALID_UNICODE_HEX,
        PARSE_INVALID_UNICODE_SURROGATE,
        PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
        PARSE_MISS_KEY,
        PARSE_MISS_COLON,
        PARSE_MISS_COMMA_OR_CURLY_BRACKET,
    };

    struct member;

    struct Value {
        union {
            struct {
                char *str;
                size_t len;
            };  // string
            struct {
                Value *arr;
                size_t a_size;
            };  // array
            struct {
                member *m;
                size_t m_size;
            };  // object
            double num;
        };
        Type type;
    };

    struct member {
        char *k;
        size_t k_len;
        Value v;
    };


#define init(v) do {(v).type = NUL; } while(0)

    void value_free(Value &v);

    int parse(Value &v, const char *json);

    Type get_type(const Value &v);

#define set_null(v) value_free(v)

    int get_boolean(const Value &v);

    void set_boolean(Value &v, bool flag);

    double get_number(const Value &v);

    void set_number(Value &v, double nu);

    typedef Value const value;

    const char *get_string(const value &v);

    size_t get_string_length(Value &v);

    void set_string(Value &v, const char *s, size_t len);

    size_t get_array_size(const Value &v);

    Value *get_array_element(const Value &v, size_t index);

    size_t get_object_size(const Value &v);

    const char * get_object_key(const Value &v, size_t index);

    size_t get_object_key_length(const Value &v, size_t index);

    Value * get_object_value(const Value &v, size_t index);

    char * stringify(const Value&v, size_t &len);

}

#endif //CPPTINYJSON_TINY_JSON_H
