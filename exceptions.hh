#pragma once

#include <exception>
#include <stdexcept>
#include <string>

namespace allocator {

    namespace exception {
        struct OutOfMemory : std::runtime_error {
            OutOfMemory(std::string &&s) : std::runtime_error(std::move(s)) {};
        };
        struct LibcFault : std::runtime_error {
            LibcFault(std::string &&s) : std::runtime_error(std::move(s)) {};
        };
        struct MemoryFormatFault : std::runtime_error {
            MemoryFormatFault(std::string &&s) : std::runtime_error(std::move(s)) {};
        };
        struct OutOfBound : std::runtime_error {
            OutOfBound(std::string &&s) : std::runtime_error(std::move(s)) {};
        };
    }

}//!allocator
