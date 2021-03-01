#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace engine {
class Shader {
public:
    static Shader from_files(const std::filesystem::path& vertex, const std::filesystem::path& fragment,
                             const std::optional<std::filesystem::path>& geometry = std::nullopt);

public:
    Shader(std::string vertex, std::string fragment, std::optional<std::string> geometry = std::nullopt);

    void init();
    void activate() const;
    int get_program_id() const;

private:
    std::string vertex_;
    std::string fragment_;
    std::optional<std::string> geometry_;

    int program_ = -1;
};
}  // namespace engine
