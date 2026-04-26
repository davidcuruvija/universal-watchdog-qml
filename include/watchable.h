#pragma once

#include <string>
#include <utility>

class Watchable {
public:
    virtual ~Watchable() = default;

    virtual std::string name() const = 0;

    virtual bool isAlive() const = 0;
};