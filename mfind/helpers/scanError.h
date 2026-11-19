#pragma once
#include <stdexcept>
// A filesystem snapshot became invalid while a scan was in progress.
class RetryScan : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
