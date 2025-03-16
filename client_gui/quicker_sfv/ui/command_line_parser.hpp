/*
 *   QuickerSFV - A fast checksum verifier
 *   Copyright (C) 2025  Andreas Weis (quickersfv@andreas-weis.net)
 *
 *   This file is part of QuickerSFV.
 *
 *   QuickerSFV is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   QuickerSFV is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef INCLUDE_GUARD_QUICKER_SFV_GUI_UI_COMMAND_LINE_PARSER_HPP
#define INCLUDE_GUARD_QUICKER_SFV_GUI_UI_COMMAND_LINE_PARSER_HPP

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/string_utilities.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace quicker_sfv::gui {

/** A command line lexer.
 * This function splits a single command line string into individual commands.
 * It mainly takes care of handling quotes and escapes in the command line string to
 * get consistent behaviour with how Windows usually handles command line arguments.
 * @param[in] str The complete command line as passed to lpCmdLine in WinMain.
 * @return A list of split command line arguments.
 */
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
            case '\"':
                status = Status::InQuotes;
                break;
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

/** Command line arguments.
 */
struct CommandLineOptions {
    std::vector<std::u16string> filesToCheck;       ///< List of files to verify.
    std::u16string outFile;                         ///< File to redirect output to.

    friend bool operator==(CommandLineOptions const&, CommandLineOptions const&) = default;
    friend bool operator!=(CommandLineOptions const&, CommandLineOptions const&) = default;
};

/** Parses a single command line string to a CommandLineOptions object.
 * @param[in] str The complete command line as passed to lpCmdLine in WinMain.
 * @return The CommandLineOptions for the command line input.
 */
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
