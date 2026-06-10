#include "keyboard_input_x11.hpp"

#include <logger.hpp>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <string>

namespace vkBasalt
{
    uint32_t convertToKeySymX11(const std::string& key)
    {
        // TODO what if X11 isn't loaded?
        const auto result = static_cast<uint32_t>(XStringToKeysym(key.c_str()));
        if (result == 0U)
        {
            Logger::err("invalid key");
        }
        return result;
    }

    bool isKeyPressedX11(uint32_t keySymbol)
    {
        static int usesX11 = -1;

        static std::unique_ptr<Display, std::function<void(Display*)>> display;

        if (usesX11 < 0)
        {
            const char* disVar = std::getenv("DISPLAY");
            if ((disVar == nullptr) || (std::strlen(disVar) == 0))
            {
                usesX11 = 0;
                Logger::debug("no X11 support");
            }
            else
            {
                display =
                    std::unique_ptr<Display, std::function<void(Display*)>>(XOpenDisplay(disVar), [](Display* display) { XCloseDisplay(display); });
                usesX11 = 1;
                Logger::debug("X11 support");
            }
        }

        if (usesX11 == 0)
        {
            return false;
        }

        std::array<char, 32U> keys_return{};

        XQueryKeymap(display.get(), std::data(keys_return));

        const KeyCode kc2 = XKeysymToKeycode(display.get(), static_cast<KeySym>(keySymbol));

        return (keys_return[kc2 >> 3] & (1 << (kc2 & 7))) != 0;
    }

} // namespace vkBasalt
