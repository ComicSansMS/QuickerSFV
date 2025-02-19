#include <quicker_sfv/sfv_file.hpp>

#include <fstream>
#include <string>
#include <string_view>

struct Ex {};

SfvFile::SfvFile(std::filesystem::path filename) {
    std::ifstream fin(filename);
    m_basePath = filename.remove_filename();
    std::string l;
    while (std::getline(fin, l)) {
        std::string_view line = l;
        // skip comments
        if (line.starts_with(";")) { continue; }
        std::size_t const separator_idx = line.find('*');
        if (separator_idx == std::string::npos) { throw Ex{}; /* @todo error */ }
        // @todo utf-8 filenames
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

void SfvFile::serialize(std::filesystem::path out_filename) const {
    std::ofstream fout(out_filename);
    for (auto const& f : m_files) {
        auto const u8str = f.path.u8string();
        fout << std::format("{} {}\n", reinterpret_cast<char const*>(u8str.c_str()), f.md5);
    }
}

void SfvFile::addEntry(std::filesystem::path p, MD5Digest md5) {
    m_files.emplace_back(std::move(p), md5);
}
