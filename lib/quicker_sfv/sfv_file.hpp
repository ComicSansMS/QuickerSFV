#ifndef INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP
#define INCLUDE_GUARD_QUICKER_SFV_SFV_FILE_HPP

#include <quicker_sfv/md5_digest.hpp>

#include <filesystem>
#include <span>
#include <vector>

class SfvFile {
public:
    struct Entry {
        std::filesystem::path path;
        MD5Digest md5;
    };
private:
    std::vector<Entry> m_files;
    std::filesystem::path m_basePath;
public:
    SfvFile() = default;

    SfvFile(std::filesystem::path file);

    std::span<const Entry> getEntries() const;

    std::filesystem::path getBasePath() const;
};

#endif
