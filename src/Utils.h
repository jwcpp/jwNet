#pragma once

namespace base
{
    class Nocopyable
    {
    public:
        Nocopyable(void) { }

    private:
        Nocopyable(Nocopyable const&) { }
        Nocopyable& operator = (Nocopyable const&) { return *this; }
    };
};