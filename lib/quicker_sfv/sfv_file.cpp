#include <quicker_sfv/sfv_file.hpp>

#include <fstream>
#include <string>
#include <string_view>

struct Ex {};

SfvFile::SfvFile(std::filesystem::path file) {
    std::ifstream fin(file);
    m_basePath = file.remove_filename();
    std::string l;
    while (std::getline(fin, l)) {
        std::string_view line = l;
        // skip comments
        if (line.starts_with(";")) { continue; }
        std::size_t const separator_idx = line.find('*');
        if (separator_idx == std::string::npos) { throw Ex{}; /* @todo error */ }
        m_files.emplace_back(
            std::filesystem::path{ line.substr(separator_idx + 1) },
            MD5Digest::fromString(line.substr(0, separator_idx))
        );
    }
}

std::span<const SfvFile::Entry> SfvFile::getEntries() const {
    return m_files;
}

std::filesystem::path SfvFile::getBasePath() const {
    return m_basePath;
}
