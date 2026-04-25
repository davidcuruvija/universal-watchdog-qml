#pragma once
#include <string>

class Watchable {
public:
    virtual ~Watchable() = default;
    virtual bool isAlive() const = 0;
    virtual std::string name() const = 0;
};