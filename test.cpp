//
// Created by 晚风吹行舟 on 2023/5/8.
//
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "tiny_json.h"


using namespace tiny_json;

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {                                                 \
        test_count++;                                    \
        if (equality)                                    \
            test_pass++;                                 \
        else {                                           \
            fprintf(stderr, "%s:%d expect: " format " actual: " format "\n", \
            __FILE__, __LINE__, expect, actual);         \
            main_ret = 1;                                \
        }                                                \
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, \
    actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect == actual), expect,\
    actual, "%.17g")

#define EXPECT_EQ_STRING(expect, actual) EXPECT_EQ_BASE( strlen(expect) == strlen(actual) && \
    memcmp(expect, actual, strlen(expect)) == 0, expect, actual, "%s")

#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_INT(expect, actual)

static void test_parse_null() {
    Value v;
    v.type = FALSE;
    EXPECT_EQ_INT(PARSE_OK, parse(v, "null"));
    EXPECT_EQ_INT(NUL, get_type(v));
}

static void test_parse_false() {
    Value v;
    v.type = TRUE;
    EXPECT_EQ_INT(PARSE_OK, parse(v, "false"));
    EXPECT_EQ_INT(FALSE, get_type(v));
}


static void test_parse_true() {
    Value v;
    v.type = FALSE;
    EXPECT_EQ_INT(PARSE_OK, parse(v, "true"));
    EXPECT_EQ_INT(TRUE, get_type(v));
}


#define TEST_NUMBER(expect, json) \
    do {                          \
        Value v;                  \
        EXPECT_EQ_INT(PARSE_OK, parse(v, json)); \
    }while(0)                     \


static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000");
    TEST_NUMBER(0.0, "1.13e+123456");      /* must underflow */

    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}


#define TEST_STRING(expect, json)\
    do {\
        Value v;\
        init(v);\
        EXPECT_EQ_INT(PARSE_OK, parse(v, json));\
        EXPECT_EQ_INT(STRING, get_type(v));\
        EXPECT_EQ_STRING(expect, get_string(v));\
        value_free(v);\
    } while(0)

static void test_parse_string() {

    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");

    TEST_STRING("Hello\0World", "\"Hello\\u0000World\"");
    TEST_STRING("\x24", "\"\\u0024\"");         /* Dollar sign U+0024 */
    TEST_STRING("\xC2\xA2", "\"\\u00A2\"");     /* Cents sign U+00A2 */
    TEST_STRING("\xE2\x82\xAC", "\"\\u20AC\""); /* Euro sign U+20AC */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\uD834\\uDD1E\"");  /* G clef sign U+1D11E */
    TEST_STRING("\xF0\x9D\x84\x9E", "\"\\ud834\\udd1e\"");  /* G clef sign U+1D11E */
    TEST_STRING("\n", "\"\\u000A\"");
}

static void test_parse_array() {
    Value v;
    init(v);
    EXPECT_EQ_INT(PARSE_OK, parse(v, "[ ]"));
    EXPECT_EQ_INT(ARRAY, get_type(v));
    EXPECT_EQ_SIZE_T(0, get_array_size(v));
    value_free(v);

    init(v);
    EXPECT_EQ_INT(PARSE_OK, parse(v, "[123, 456]"));
    EXPECT_EQ_INT(ARRAY, get_type(v));
    EXPECT_EQ_SIZE_T(2, get_array_size(v));
    EXPECT_EQ_DOUBLE(123.0, get_array_element(v, 0)->num);
    EXPECT_EQ_DOUBLE(456.0, get_array_element(v, 1)->num);
    value_free(v);


    init(v);
    EXPECT_EQ_INT(PARSE_OK, parse(v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(ARRAY, get_type(v));
    EXPECT_EQ_SIZE_T(5, get_array_size(v));
    EXPECT_EQ_INT(NUL, get_type(*get_array_element(v, 0)));
    EXPECT_EQ_INT(FALSE, get_type(*get_array_element(v, 1)));
    EXPECT_EQ_INT(TRUE, get_type(*get_array_element(v, 2)));
    EXPECT_EQ_DOUBLE(123.0, get_number(*get_array_element(v, 3)));
    EXPECT_EQ_INT(3, get_string_length(*get_array_element(v, 4)));
    EXPECT_EQ_STRING("abc", get_string(*get_array_element(v, 4)));
    value_free(v);

    init(v);
    EXPECT_EQ_INT(PARSE_OK, parse(v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(ARRAY, get_type(v));
    EXPECT_EQ_INT(4, get_array_size(v));
    for (int i = 0; i < 4; i++) {
        Value tmp = *get_array_element(v, i);
        EXPECT_EQ_INT(ARRAY, get_type(tmp));
        EXPECT_EQ_SIZE_T(i, get_array_size(tmp));
        for (int j = 0; j < i; j++) {
            Value tmp2 = *get_array_element(tmp, j);
            EXPECT_EQ_INT(NUMBER, get_type(tmp2));
            EXPECT_EQ_DOUBLE(double (j), get_number(tmp2));
        }
    }
    value_free(v);
}

static void test_parse_object() {
    Value v;
    size_t i;

    init(v);

    EXPECT_EQ_INT(PARSE_OK, parse(v, " { } "));
    EXPECT_EQ_INT(OBJECT, get_type(v));
    EXPECT_EQ_SIZE_T(0, get_object_size(v));
    value_free(v);

    init(v);
    EXPECT_EQ_INT(PARSE_OK, parse(v, "{\"n\": 123, \"123\" : [] }"));
    value_free(v);

    init(v);
    EXPECT_EQ_INT(PARSE_OK, parse(v,
                                  " { "
                                  "\"n\" : null , "
                                  "\"f\" : false , "
                                  "\"t\" : true , "
                                  "\"i\" : 123 , "
                                  "\"s\" : \"abc\", "
                                  "\"a\" : [ 1, 2, 3 ],"
                                  "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
                                  " } "
    ));
    EXPECT_EQ_INT(OBJECT, get_type(v));
    EXPECT_EQ_SIZE_T(7, get_object_size(v));
    EXPECT_EQ_INT(1, get_object_key_length(v, 0));
    EXPECT_EQ_STRING("n", get_object_key(v, 0));
    EXPECT_EQ_INT(NUL, get_type(*get_object_value(v, 0)));
    EXPECT_EQ_INT(1, get_object_key_length(v, 1));
    EXPECT_EQ_STRING("f", get_object_key(v, 1));
    EXPECT_EQ_INT(FALSE, get_type(*get_object_value(v, 1)));
    EXPECT_EQ_INT(1, get_object_key_length(v, 2));
    EXPECT_EQ_STRING("t", get_object_key(v, 2));
    EXPECT_EQ_INT(TRUE, get_type(*get_object_value(v, 2)));
    EXPECT_EQ_INT(1, get_object_key_length(v, 3));
    EXPECT_EQ_STRING("i", get_object_key(v, 3));
    EXPECT_EQ_INT(NUMBER, get_type(*get_object_value(v, 3)));
    EXPECT_EQ_DOUBLE(123.0, get_number(*get_object_value(v, 3)));
    EXPECT_EQ_INT(1, get_object_key_length(v, 4));
    EXPECT_EQ_STRING("s", get_object_key(v, 4));
    EXPECT_EQ_INT(STRING, get_type(*get_object_value(v, 4)));
    EXPECT_EQ_STRING("abc", get_string(*get_object_value(v, 4)));
    EXPECT_EQ_INT(1, get_object_key_length(v, 5));
    EXPECT_EQ_STRING("a", get_object_key(v, 5));
    EXPECT_EQ_INT(3, get_array_size(*get_object_value(v, 5)));
    for (int i = 1; i <= 3; i++) {
        Value tmp = *get_array_element(*get_object_value(v, 5), i - 1);
        EXPECT_EQ_INT(NUMBER, get_type(tmp));
        EXPECT_EQ_DOUBLE(double(i), get_number(tmp));
    }
    EXPECT_EQ_INT(1, get_object_key_length(v, 6));
    EXPECT_EQ_STRING("o", get_object_key(v, 6));
    Value tmp2 = *get_object_value(v, 6);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ_INT(1, get_object_key_length(tmp2, i));
        EXPECT_EQ_INT(i + '1', get_object_key(tmp2, i)[0]);
        EXPECT_EQ_INT(NUMBER, get_type(*get_object_value(tmp2, i)));
        EXPECT_EQ_DOUBLE(1.0 + i, get_number(*get_object_value(tmp2, i)));
    }
    value_free(v);
}

#define TEST_ROUNDTRIP(json)\
    do {\
        Value v;\
        char* json2;\
        size_t length;\
        init(v);\
        EXPECT_EQ_INT(PARSE_OK, parse(v, json));\
        json2 = stringify(v, length);\
        EXPECT_EQ_STRING(json, json2);\
        value_free(v);\
        free(json2);\
    } while(0)


static void test_stringify_number() {
    TEST_ROUNDTRIP("0");
    TEST_ROUNDTRIP("-0");
    TEST_ROUNDTRIP("1");
    TEST_ROUNDTRIP("-1");
    TEST_ROUNDTRIP("1.5");
    TEST_ROUNDTRIP("-1.5");
    TEST_ROUNDTRIP("3.25");
    TEST_ROUNDTRIP("1e+20");
    TEST_ROUNDTRIP("1.234e+20");
    TEST_ROUNDTRIP("1.234e-20");


    TEST_ROUNDTRIP("1.0000000000000002"); /* the smallest number > 1 */
    TEST_ROUNDTRIP("4.9406564584124654e-324"); /* minimum denormal */
    TEST_ROUNDTRIP("-4.9406564584124654e-324");
    TEST_ROUNDTRIP("2.2250738585072009e-308");  /* Max subnormal double */
    TEST_ROUNDTRIP("-2.2250738585072009e-308");
    TEST_ROUNDTRIP("2.2250738585072014e-308");  /* Min normal positive double */
    TEST_ROUNDTRIP("-2.2250738585072014e-308");
    TEST_ROUNDTRIP("1.7976931348623157e+308");  /* Max double */
    TEST_ROUNDTRIP("-1.7976931348623157e+308");
}

static void test_stringify_string() {
    TEST_ROUNDTRIP("\"\"");
    TEST_ROUNDTRIP("\"Hello\"");
    TEST_ROUNDTRIP("\"Hello\\nWorld\"");
    TEST_ROUNDTRIP("\"\\\" \\\\ / \\b \\f \\n \\r \\t\"");
    TEST_ROUNDTRIP("\"Hello\\u0000World\"");
}

static void test_stringify_array() {
    TEST_ROUNDTRIP("[]");
    TEST_ROUNDTRIP("[null,false,true,123,\"abc\",[1,2,3]]");
}

static void test_stringify_object() {
    TEST_ROUNDTRIP("{}");
    TEST_ROUNDTRIP(
            "{\"n\":null,\"f\":false,\"t\":true,\"i\":123,\"s\":\"abc\",\"a\":[1,2,3],\"o\":{\"1\":1,\"2\":2,\"3\":3}}");
}


#define TEST_ERROR(error, json)   \
    do {                          \
        Value v;                  \
        v.type = FALSE;           \
        EXPECT_EQ_INT(error, parse(v, json)); \
        EXPECT_EQ_INT(NUL, get_type(v));      \
    } while(0)

static void test_parse_expect_value() {
    TEST_ERROR(PARSE_EXPECT_VALUE, "");
    TEST_ERROR(PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(PARSE_INVALID_VALUE, "?");
    TEST_ERROR(PARSE_INVALID_VALUE, "nul");

    TEST_ERROR(PARSE_INVALID_VALUE, "1.");
    TEST_ERROR(PARSE_INVALID_VALUE, "+1.2");
    TEST_ERROR(PARSE_INVALID_VALUE, "-.23");
    TEST_ERROR(PARSE_INVALID_VALUE, "0220");
    TEST_ERROR(PARSE_INVALID_VALUE, "-1.");
    TEST_ERROR(PARSE_INVALID_VALUE, "-00.123");
    TEST_ERROR(PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(PARSE_INVALID_VALUE, "nan");

    TEST_ERROR(PARSE_INVALID_VALUE, "[1,]");
    TEST_ERROR(PARSE_INVALID_VALUE, "[\"a\", nul]");

}

static void test_parse_root_not_singular() {
    TEST_ERROR(PARSE_ROOT_NOT_SINGULAR, "null x");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"123");
    TEST_ERROR(PARSE_MISS_QUOTATION_MARK, "\"");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
    TEST_ERROR(PARSE_INVALID_STRING_CHAR, "\"\n\"");
}


static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}


static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1, 1.5");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_miss_key() {
    TEST_ERROR(PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(PARSE_MISS_COLON, "{\"a\",\"b\"}");
    TEST_ERROR(PARSE_MISS_COLON, "{\"a\";1}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":123");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_access_null() {
    Value v;
    init(v);
    set_string(v, "a", 1);
    set_null(v);
    EXPECT_EQ_INT(NUL, get_type(v));
    value_free(v);
}

static void test_access_number() {
    Value v;
    init(v);
    v.type = NUMBER;
    v.num = 12.3;
    EXPECT_EQ_DOUBLE(12.3, get_number(v));
}

static void test_access_boolean() {
    Value v;
    init(v);
    v.type = TRUE;
    EXPECT_EQ_INT(TRUE, get_boolean(v));
    v.type = FALSE;
    EXPECT_EQ_INT(FALSE, get_boolean(v));
}

static void test_access_string() {
    Value v;
    init(v);
    set_string(v, "123", 3);
    EXPECT_EQ_STRING("123", get_string(v));
    set_string(v, "", 0);
    EXPECT_EQ_STRING("", get_string(v));
    value_free(v);
}

static void test_str() {
    const char *p = "";
    bool x = p[0] == '\0';
    EXPECT_EQ_INT(1, x);
}

static void test_strtod() {
    char *end;
    double x = strtod("00123", &end);
    EXPECT_EQ_DOUBLE(x, 0.0);
}


static void test_parse() {
    test_parse_null();
    test_parse_number();
    test_parse_string();
    test_parse_array();
    test_parse_object();

    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_invalid_string_escape();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_char();
    test_parse_invalid_unicode_hex();
    test_parse_invalid_unicode_surrogate();
    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}

static void test_access() {

    test_access_boolean();
    test_access_null();
    test_access_number();
    test_access_string();
}

static void test_stringify() {
    TEST_ROUNDTRIP("null");
    TEST_ROUNDTRIP("false");
    TEST_ROUNDTRIP("true");
    test_stringify_number();
    test_stringify_string();
    test_stringify_object();
    test_stringify_object();
}

int main() {

    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    test_parse();
    test_access();
    test_stringify();

    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    _CrtDumpMemoryLeaks();
    return main_ret;
}