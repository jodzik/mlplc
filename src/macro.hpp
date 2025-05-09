#pragma once

#include <mlplc/exception.hpp>

#ifndef STR
#define XSTR(x) #x
#define STR(x) XSTR(x)
#endif

#define MACRO_POSITION_SHORT    STR(__func__) "(): " STR(__LINE__)

#define CCALL_UNTIL(expr) do {int const _rc = expr; if (0 == _rc) {break;}} while (1)

#define CCALL(expr) do {int const _rc = expr; if (0 != _rc) { \
    throw Exception(std::string_view(MACRO_POSITION_SHORT ": " STR(expr) " failed: "), \
    exception_type_from_c_code(_rc), _rc);}} while (0)

// ASSERT(bool expression, ExceptionType:: ...)
#define ASSERT(expr, ...) do {if (!(expr)) { \
    throw Exception(std::string_view(MACRO_POSITION_SHORT  ": " STR(expr) " failed: "), ## __VA_ARGS__);}} while (0)
