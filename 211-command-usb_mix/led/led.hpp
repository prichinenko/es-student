#pragma once
#include <stdint.h>
#include "pico/types.h"

class Led
{
public:
    explicit Led(uint pin);
    void set(bool on);
    bool is_on() const;
    void toggle();

private:
    uint pin;
    bool on;
};
