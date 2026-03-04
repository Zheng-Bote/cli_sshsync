#ifndef ENV_PARSER_HPP
#define ENV_PARSER_HPP

#include <string>
#include <unordered_map>
#include <optional>

class EnvParser {
public:
    explicit EnvParser(const std::string& env_filepath);

    bool load();
    std::optional<std::string> getPassword(const std::string& username) const;
    std::optional<std::string> getTargetDir(const std::string& host) const;
    std::optional<std::string> getValue(const std::string& key) const;

private:
    std::string filepath_;
    std::unordered_map<std::string, std::string> env_map_;
};

#endif // ENV_PARSER_HPP
