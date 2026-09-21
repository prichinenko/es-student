#include "led.hpp"
#include "led.h"
#include "pico/stdlib.h"

// ---------------------------------------------------------------------------
// Реализация класса
// ---------------------------------------------------------------------------
Led::Led(uint pin) : pin(pin), on(false)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
}

void Led::set(bool on)
{
    this->on = on;
    gpio_put(pin, on ? 1 : 0);
}

bool Led::is_on() const { return on; }

void Led::toggle() { set(!on); }

// ---------------------------------------------------------------------------
// C++-мостик. Даёт C-файлу единственный способ достать до класса.
// Не попадает в публичный C-заголовок -> не виден остальному C-коду.
// ---------------------------------------------------------------------------
struct led_s
{
    Led obj;
    explicit led_s(uint pin) : obj(pin) {}
};

extern "C"
{

    led_t *led_bridge_create(uint pin)
    {
        return reinterpret_cast<led_t *>(new led_s(pin));
    }

    void led_bridge_destroy(led_t *self)
    {
        delete reinterpret_cast<led_s *>(self);
    }

    void led_bridge_set(led_t *self, bool on)
    {
        reinterpret_cast<led_s *>(self)->obj.set(on);
    }

    bool led_bridge_is_on(const led_t *self)
    {
        return reinterpret_cast<const led_s *>(self)->obj.is_on();
    }
    
    void led_bridge_toggle(led_t *self)
    {
        reinterpret_cast<led_s *>(self)->obj.toggle();
    }

} // extern "C"