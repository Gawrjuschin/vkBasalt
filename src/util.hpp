#ifndef UTIL_HPP_INCLUDED
#define UTIL_HPP_INCLUDED

#include <string>
#include <string_view>
#include <sstream>

namespace vkBasalt
{
    enum class Color
    {
        defaultColor,

        black,
        red,
        green,
        yellow,
        blue,
        magenta,
        cyan,
        white
    };

    void outputInColor(std::string_view output, Color foreground = Color::defaultColor, Color background = Color::defaultColor);

    template<typename T>
    std::string convertToString(const T& object)
    {
        std::stringstream ss;
        ss << object;
        return ss.str();
    }
} // namespace vkBasalt

#endif // UTIL_HPP_INCLUDED
