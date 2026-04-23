#pragma once
#include "watchable.h"

class AlwaysAliveWatchable : public Watchable {
public:
    bool isAlive() override { 
        return true; 
    }

    std::string name() const override { 
        return "always-alive"; 
    }
};