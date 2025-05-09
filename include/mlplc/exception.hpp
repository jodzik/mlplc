#pragma once

#include <magic_enum/magic_enum.hpp>

#include <exception>
#include <string_view>
#include <string>
#include <cstdint>
#include <sstream>

namespace mlplc {

enum class ExceptionType : int32_t {
    NoMemory,
    NoDev,
    Unknown,
    DeviceAlreadyInUse,
    PortAlreadyInUse,
    ObjectMoved,
    DestroyingRunningThread,
};

constexpr ExceptionType exception_type_from_c_code(int error_code) {
    return ExceptionType::Unknown;
} 


class Exception : public std::exception {
public:
    Exception(std::string_view where, ExceptionType type) :
        _where(where),
        _type(type)
    {
        std::stringstream ss;
        ss << this->_where << ": " << magic_enum::enum_name(this->_type);
        this->_what = ss.str();
    }

    template<typename... Args>
    Exception(std::string_view where, ExceptionType type, Args... args) :
        _where(where),
        _type(type)
    {
        std::stringstream ss;
        ss << this->_where << ": " << magic_enum::enum_name(this->_type);

        ([&] {
            ss << args;
        } (), ...);

        this->_what = ss.str();
    }

    virtual char const* what() const noexcept override {
        return this->_what.c_str();
    }

    ExceptionType type() const {
        return this->_type;
    }

private:
    std::string_view _where;
    ExceptionType _type;
    std::string _what = "";
};

} // namespace mlplc