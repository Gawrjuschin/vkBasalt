#ifndef CONFIG_HPP_INCLUDED
#define CONFIG_HPP_INCLUDED

#include <cstdint>
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

namespace vkBasalt
{
    class Config
    {
    public:
        Config();

        Config(const Config& other)            = default;
        Config& operator=(const Config& other) = default;

        Config(Config&&)            = default;
        Config& operator=(Config&&) = default;

        ~Config() = default;

        template<typename T>
        T getOption(const std::string& option, const T& defaultValue = {})
        {
            T result = defaultValue;
            parseOption(option, result);
            return result;
        }

    private:
        std::unordered_map<std::string, std::string> options;

        void readConfigLine(std::string line);
        void readConfigFile(std::istream& stream);

        void parseOption(const std::string& option, int32_t& result);
        void parseOption(const std::string& option, float& result);
        void parseOption(const std::string& option, bool& result);
        void parseOption(const std::string& option, std::string& result);
        void parseOption(const std::string& option, std::vector<std::string>& result);
    };
} // namespace vkBasalt

#endif // CONFIG_HPP_INCLUDED
