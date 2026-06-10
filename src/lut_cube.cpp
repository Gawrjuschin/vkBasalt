#include "lut_cube.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <spanstream>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

#include <logger.hpp>

namespace vkBasalt
{
    namespace
    {
        std::string skipWhiteSpace(std::string text)
        {

            if (const auto notSpaceIt =
                    std::ranges::find_if_not(text, [](const auto character) { return std::isspace(static_cast<uint8_t>(character)) != 0; });
                notSpaceIt != std::cend(text))
            {
                text.erase(std::begin(text), notSpaceIt);
            }

            return text;
        }
    } // namespace

    LutCube::LutCube() = default;

    LutCube::LutCube(const std::string& file)
    {
        std::ifstream cubeStream(file);
        if (!cubeStream.good())
        {
            Logger::err("lut cube file does not exist");
            return;
        }

        std::string line;

        while (std::getline(cubeStream, line))
        {
            parseLine(line);
        }
    }
    void LutCube::parseLine(std::string line)
    {
        using namespace std::string_view_literals;
        constexpr static auto lut3dSize = "LUT_3D_SIZE"sv;
        constexpr static auto domainMin = "DOMAIN_MIN"sv;
        constexpr static auto domainMax = "DOMAIN_MAX"sv;

        if (std::empty(line) || line.front() == '#')
        {
            return;
        }
        if (const auto lut3dSizePos = line.find(lut3dSize); lut3dSizePos != std::string::npos)
        {
            line = line.substr(lut3dSizePos + std::size(lut3dSize));
            line = skipWhiteSpace(std::move(line));
            size = std::stoi(line);
            const auto sizeUL = static_cast<size_t>(size);

            colorCube = std::vector<uint8_t>(sizeUL * sizeUL * sizeUL * 4U, uint8_t{0xFF});
            return;
        }
        if (const auto domainMinPos = line.find(domainMin); domainMinPos != std::string::npos)
        {
            line = line.substr(domainMinPos + std::size(domainMin));
            splitTripel(line, minX, minY, minZ);
            return;
        }
        if (const auto domainMaxPos = line.find(domainMax); domainMaxPos != std::string::npos)
        {
            line = line.substr(domainMaxPos + std::size(domainMax));
            splitTripel(line, maxX, maxY, maxZ);
            return;
        }
        if (std::isdigit(static_cast<uint8_t>(line.front())) != 0)
        {
            float         x{}, y{}, z{};
            uint8_t       outX{}, outY{}, outZ{};
            splitTripel(line, x, y, z);
            clampTripel(x, y, z, outX, outY, outZ);
            writeColor(currentX, currentY, currentZ, outX, outY, outZ);
            if (currentX != size - 1)
            {
                currentX++;
            }
            else if (currentY != size - 1)
            {
                currentY++;
                currentX = 0;
            }
            else if (currentZ != size - 1)
            {
                currentZ++;
                currentX = 0;
                currentY = 0;
            }
            return;
        }
    }

    void LutCube::splitTripel(std::string tripel, float& x, float& y, float& z)
    {
        // TODO: test new impl
        if constexpr (constexpr bool modernSolution{false})
        {
            // TODO: std::from_chars based solution
            std::spanstream sstream{tripel};
            sstream >> x >> y >> z;
        }
        else
        {
            tripel       = skipWhiteSpace(std::move(tripel));
            size_t after = tripel.find_first_of(" \n");
            x            = std::stof(tripel.substr(0, after));
            tripel       = tripel.substr(after);

            tripel = skipWhiteSpace(std::move(tripel));
            after  = tripel.find_first_of(" \n");
            y      = std::stof(tripel.substr(0, after));
            tripel = tripel.substr(after);

            tripel = skipWhiteSpace(std::move(tripel));
            z      = std::stof(tripel);
        }
    }

    void LutCube::clampTripel(float x, float y, float z, unsigned char& outX, unsigned char& outY, unsigned char& outZ) const
    {
        // TODO: test new impl
        if constexpr (constexpr bool modernSolution{false})
        {
            const auto multX = x / (maxX - minX);
            const auto multY = y / (maxY - minY);
            const auto multZ = z / (maxZ - minZ);

            outX = static_cast<uint8_t>(float{std::numeric_limits<uint8_t>::max()} * multX);
            outY = static_cast<uint8_t>(float{std::numeric_limits<uint8_t>::max()} * multY);
            outZ = static_cast<uint8_t>(float{std::numeric_limits<uint8_t>::max()} * multZ);
        }
        else
        {
            outX = (uint8_t) 0xFF * (x / (maxX - minX));
            outY = (uint8_t) 0xFF * (y / (maxY - minY));
            outZ = (uint8_t) 0xFF * (z / (maxZ - minZ));
        }
    }

    void LutCube::writeColor(int x, int y, int z, unsigned char r, unsigned char g, unsigned char b)
    {
        static constexpr int colorSize = 4; // 4 bytes per point in the cube, rgba

        const int locationR = (((z * size) + y) * size + x) * colorSize;

        colorCube[locationR + 0] = r;
        colorCube[locationR + 1] = g;
        colorCube[locationR + 2] = b;
    }
} // namespace vkBasalt
