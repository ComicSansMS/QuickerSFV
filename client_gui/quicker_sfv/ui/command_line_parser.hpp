#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_COMMAND_LINE_PARSER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_COMMAND_LINE_PARSER_HPP

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/utf.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace quicker_sfv::gui {

inline std::vector<std::u8string> commandLineLexer(std::u8string_view str) {
    std::vector<std::u8string> args;
    enum class Status {
        StartOfArg,
        InArg,
        InQuotes,
    } status = Status::StartOfArg;
    args.emplace_back();
    auto end_of_arg = [&args]() {
        if (!checkValidUtf8(std::span<std::byte const>(
                reinterpret_cast<std::byte const*>(args.back().data()), args.back().size()))) {
            quicker_sfv::throwException(quicker_sfv::Error::ParserError);
        }
        args.emplace_back();
    };
    for (std::u8string_view::size_type i = 0; i < str.size(); ++i) {
        char8_t const c = str[i];
        if (c == u8'\\') {
            if (i + 1 < str.size()) {
                if (str[i + 1] == u8'\\') {
                    args.back().push_back(u8'\\');
                    ++i;
                    continue;
                } else if (str[i + 1] == u8'"') {
                    args.back().push_back(u8'"');
                    ++i;
                    continue;
                }
            }
        }
        switch (status) {
        case Status::StartOfArg:
            switch (c) {
            case u8' ': [[fallthrough]];
            case u8'\t':
                // skip whitespace
                break;
            case u8'"':
                status = Status::InQuotes;
                break;
            default:
                status = Status::InArg;
                args.back().push_back(c);
                break;
            }
            break;
        case Status::InArg:
            switch (c) {
            case '\"': [[fallthrough]];
            case u8' ': [[fallthrough]];
            case u8'\t':
                end_of_arg();
                status = Status::StartOfArg;
                break;
            default:
                args.back().push_back(c);
                break;
            }
            break;
        case Status::InQuotes:
            switch (c) {
            case u8'"':
                // double quotes are interpreted as literal quotes
                if ((i + 1 < str.size()) && (str[i + 1] == u8'"')) {
                    args.back().push_back(u8'"');
                    ++i;
                } else {
                    end_of_arg();
                    status = Status::StartOfArg;
                }
                break;
            default:
                args.back().push_back(c);
                break;
            }
            break;
        }
    }
    while (!args.back().empty() && ((args.back().back() == u8' ') || (args.back().back() == u8'\t'))) {
        args.back().pop_back();
    }
    if (!args.back().empty()) { end_of_arg(); }
    args.pop_back();
    return args;
}

struct CommandLineOptions {
    std::vector<std::u16string> filesToCheck;
    std::u16string outFile;

    friend bool operator==(CommandLineOptions const&, CommandLineOptions const&) = default;
    friend bool operator!=(CommandLineOptions const&, CommandLineOptions const&) = default;
};


inline CommandLineOptions parseCommandLine(std::u8string_view str) {
    std::vector<std::u8string> const args = commandLineLexer(str);
    CommandLineOptions opts;
    for (auto const& f : args) {
        if (f == u8"DOALL") {
            // ignore DOALL option - we always verify everything
            continue;
        } else if (f.starts_with(u8"OUTPUT:")) {
            opts.outFile = convertToUtf16(f.substr(7));
            continue;
        }
        opts.filesToCheck.push_back(convertToUtf16(f));
    }
    if (!opts.outFile.empty()) {
        // in no-GUI mode, only allow checking a single file
        opts.filesToCheck.resize(1);
    }
    return opts;
}

}

#endif
