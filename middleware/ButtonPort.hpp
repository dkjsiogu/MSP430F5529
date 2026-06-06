#ifndef MIDDLEWARE_BUTTON_PORT_HPP
#define MIDDLEWARE_BUTTON_PORT_HPP

#include "CBindings.hpp"

namespace middleware {

template <typename SampleSource>
class ButtonPort {
public:
    static void init()
    {
        buttons_init();
    }

    static void task(const SampleSource& sample)
    {
        buttons_task(sample.current(), sample.hasCurrentU8());
    }

    static bool pending()
    {
        return buttons_pending() != 0u;
    }
};

} // namespace middleware

#endif
