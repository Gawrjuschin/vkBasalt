#include "util.hpp"

#include <iostream>
#include <iterator>
#include <stdio.h>
#include <string>
#include <string_view>
#include <cstdio>
#include <unistd.h>
#include <vector>

namespace vkBasalt
{
    void outputInColor(std::string_view output, Color foreground, Color background)
    {
        std::vector<std::string> magicNumbers;
        switch (foreground)
        {
            case Color::black: magicNumbers.emplace_back("30"); break;
            case Color::red: magicNumbers.emplace_back("31"); break;
            case Color::green: magicNumbers.emplace_back("32"); break;
            case Color::yellow: magicNumbers.emplace_back("33"); break;
            case Color::blue: magicNumbers.emplace_back("34"); break;
            case Color::magenta: magicNumbers.emplace_back("35"); break;
            case Color::cyan: magicNumbers.emplace_back("36"); break;
            case Color::white: magicNumbers.emplace_back("37"); break;
            default: break;
        }
        switch (background)
        {
            case Color::black: magicNumbers.emplace_back("40"); break;
            case Color::red: magicNumbers.emplace_back("41"); break;
            case Color::green: magicNumbers.emplace_back("42"); break;
            case Color::yellow: magicNumbers.emplace_back("43"); break;
            case Color::blue: magicNumbers.emplace_back("44"); break;
            case Color::magenta: magicNumbers.emplace_back("45"); break;
            case Color::cyan: magicNumbers.emplace_back("46"); break;
            case Color::white: magicNumbers.emplace_back("47"); break;
            default: break;
        }
        std::string magicString;
        for (bool first = true; auto& magicNumber : magicNumbers)
        {
            if (!first)
            {
                magicString += ";";
            }
            magicString += magicNumber;
            first = false;
        }
        if (std::empty(magicString) || (isatty(fileno(stdout)) == 0))
        {
            std::cout << output << '\n';
        }
        else
        {
            std::cout << "\033[" << magicString << "m" << output << "\033[0m" << '\n';
        }
    }
} // namespace vkBasalt
