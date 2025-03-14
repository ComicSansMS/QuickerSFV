#include <quicker_sfv/ui/command_line_parser.hpp>

#include <catch.hpp>

TEST_CASE("Command Line Parser")
{
    SECTION("Command Line Lexing") {
        using quicker_sfv::gui::commandLineLexer;
        using R = std::vector<std::u8string>;
        SECTION("No arguments") {
            CHECK(commandLineLexer(u8"") == R{});
        }
        SECTION("Arguments are delimited by whitespace, which is either a space or a tab") {
            CHECK(commandLineLexer(u8"arg1 this_is_arg2 and_finally_arg3") ==
                R{ u8"arg1", u8"this_is_arg2", u8"and_finally_arg3" });
            CHECK(commandLineLexer(u8"arg1          lots_of_spaces_before\ttab_before\t \t  \t      mixed_before") ==
                R{ u8"arg1", u8"lots_of_spaces_before", u8"tab_before", u8"mixed_before" });
            CHECK(commandLineLexer(u8" ") == R{});
            CHECK(commandLineLexer(u8"\t") == R{});
            CHECK(commandLineLexer(u8"    \t  \t   \t") == R{});
            CHECK(commandLineLexer(u8"arg1_with_trailing_space      \t   ") ==
                R{ u8"arg1_with_trailing_space" });
            CHECK(commandLineLexer(u8"arg1 arg2_with_trailing_space      \t   ") ==
                R{ u8"arg1", u8"arg2_with_trailing_space" });
        }
        SECTION("A string surrounded by double quote marks is interpreted as a single argument, which may contain "
                "white-space characters.") {
            CHECK(commandLineLexer(u8"\"arg1 still_arg1 \t and_finally_still_arg1\"") ==
                R{ u8"arg1 still_arg1 \t and_finally_still_arg1" });
            CHECK(commandLineLexer(u8"    \"arg1 still_arg1 \t and_finally_still_arg1\"   ") ==
                R{ u8"arg1 still_arg1 \t and_finally_still_arg1" });
            CHECK(commandLineLexer(u8"\t\"arg1 still_arg1 \t and_finally_still_arg1\"\t") ==
                R{ u8"arg1 still_arg1 \t and_finally_still_arg1" });
            CHECK(commandLineLexer(u8"\" arg1 still_arg1 \t and_finally_still_arg1 \"") ==
                R{ u8" arg1 still_arg1 \t and_finally_still_arg1 " });

            CHECK(commandLineLexer(u8R"("arg1 still_arg1" arg2 "arg 3 with spaces ")") ==
                R{ u8"arg1 still_arg1", u8"arg2", u8"arg 3 with spaces " });
        }
        SECTION("Trailing double quote marks are ignored") {
            CHECK(commandLineLexer(u8R"("arg1 still_arg1" arg2")") ==
                R{ u8"arg1 still_arg1", u8"arg2" });
            CHECK(commandLineLexer(u8R"("arg1 still_arg1" arg2 ")") ==
                R{ u8"arg1 still_arg1", u8"arg2" });
            CHECK(commandLineLexer(u8"\"arg1 still_arg1\" arg2\t\"") ==
                R{ u8"arg1 still_arg1", u8"arg2" });
        }

        SECTION("If the command line ends before a closing double quote mark is found, then all the characters read so far "
                "are output as the last argument.") {
            CHECK(commandLineLexer(u8"\"arg1 still_arg1 no terminating quote") ==
                R{ u8"arg1 still_arg1 no terminating quote" });
            CHECK(commandLineLexer(u8"\"arg1 still_arg1 no terminating quote     ") ==
                R{ u8"arg1 still_arg1 no terminating quote" });
            CHECK(commandLineLexer(u8"\"arg1 still_arg1\"  \t\"no terminating quote     ") ==
                R{ u8"arg1 still_arg1", u8"no terminating quote" });
        }

        SECTION("Within a quoted string, a pair of double quote marks is interpreted as a single escaped double quote mark.") {
            CHECK(commandLineLexer(u8"\"arg1 \"\" still arg1\" \t arg2") ==
                R{ u8"arg1 \" still arg1", u8"arg2" });
            CHECK(commandLineLexer(u8"\"arg1 \"\"\" \"not still arg1\" \t arg3") ==
                R{ u8"arg1 \"", u8"not still arg1", u8"arg3" });
            CHECK(commandLineLexer(u8"\"arg1 \"\"\"\" still arg1\" \t arg2") ==
                R{ u8"arg1 \"\" still arg1", u8"arg2" });
        }

        SECTION("A double quote mark preceded by a backslash (\") is interpreted as a literal double quote mark.") {
            CHECK(commandLineLexer(u8"\"arg1 \\\" still arg1\" \t arg2") ==
                R{ u8"arg1 \" still arg1", u8"arg2" });
            CHECK(commandLineLexer(u8"\"arg1 \\\"\" \"not still arg1\" \t arg3") ==
                R{ u8"arg1 \"", u8"not still arg1", u8"arg3" });
            CHECK(commandLineLexer(u8"\"arg1 \\\"\\\" still arg1\" \t arg2") ==
                R{ u8"arg1 \"\" still arg1", u8"arg2" });
        }

        SECTION("Backslashes are interpreted literally, unless they immediately precede a double quote mark.") {
            CHECK(commandLineLexer(u8R"(c:\some_folder\a.txt)") ==
                R{ u8"c:\\some_folder\\a.txt" });
            CHECK(commandLineLexer(u8"\"c:\\some folder\\a.txt\" \"d:\\some other folder\\some file.txt\"") ==
                R{ u8"c:\\some folder\\a.txt", u8"d:\\some other folder\\some file.txt"});
            CHECK(commandLineLexer(u8"\"c:\\some folder\\a.txt\" \"d:\\some \\\"other\\\" folder\\some file.txt\"") ==
                R{ u8"c:\\some folder\\a.txt", u8"d:\\some \"other\" folder\\some file.txt" });
        }

        SECTION("The caret (^) isn't recognized as an escape character or delimiter.") {
            CHECK(commandLineLexer(u8"arg1 arg2^still_arg2") ==
                R{ u8"arg1", u8"arg2^still_arg2"});
        }

        SECTION("If an even number of backslashes is followed by a double quote mark, then one backslash (\\) is placed in "
                "the result for every pair of backslashes, and the double quote mark is interpreted as a string delimiter.") {
            CHECK(commandLineLexer(u8"arg1 \"arg2 with spaces\\\\\" arg3") ==
                R{ u8"arg1", u8"arg2 with spaces\\", u8"arg3"});
            CHECK(commandLineLexer(u8"arg1 \"arg2 with spaces\\\\\\\\\" arg3") ==
                R{ u8"arg1", u8"arg2 with spaces\\\\", u8"arg3" });
            CHECK(commandLineLexer(u8"arg1 \\\\\"arg2 with spaces\\\\\" arg3") ==
                R{ u8"arg1", u8"\\" u8"arg2 with spaces\\", u8"arg3"});
        }

        SECTION("If an odd number of backslashes is followed by a double quote mark, then one backslash (\\) is placed in "
                "the result for every pair of backslashes.The double quote mark is interpreted as an escape sequence by the "
                "remaining backslash, causing a literal double quote mark to be placed in the result.") {
            CHECK(commandLineLexer(u8"arg1 \"arg2 with spaces\\\" not arg3\"") ==
                R{ u8"arg1", u8"arg2 with spaces\" not arg3" });
            CHECK(commandLineLexer(u8"arg1 \"arg2 with spaces\\\" not arg3") ==
                R{ u8"arg1", u8"arg2 with spaces\" not arg3" });
            CHECK(commandLineLexer(u8"arg1 \"arg2 with spaces\\\\\\\" not arg3\"") ==
                R{ u8"arg1", u8"arg2 with spaces\\\" not arg3" });
            CHECK(commandLineLexer(u8"arg1 \"arg2 with spaces\\\\\\\" not arg3") ==
                R{ u8"arg1", u8"arg2 with spaces\\\" not arg3" });
            CHECK(commandLineLexer(u8"arg1 \"arg2 with spaces\\\\\\\\\\\" not arg3") ==
                R{ u8"arg1", u8"arg2 with spaces\\\\\" not arg3" });
        }

        SECTION("Invalid UTF-8 triggers exception") {
            {
                char const str[] = { 'a', 'b', 'c', '\0' };
                CHECK(commandLineLexer(quicker_sfv::assumeUtf8(str)) == R{ u8"abc" });
            }
            {
                char const str[] = { 'a', static_cast<char>(0x80), 'c', '\0' };
                CHECK_THROWS_AS(commandLineLexer(quicker_sfv::assumeUtf8(str)), quicker_sfv::Exception);
            }
        }
    }

    SECTION("Command line parser") {
        using quicker_sfv::gui::parseCommandLine;
        using R = quicker_sfv::gui::CommandLineOptions;
        SECTION("Empty Command Line") {
            CHECK(parseCommandLine(u8"") == R{});
            CHECK(parseCommandLine(u8"    ") == R{});
            CHECK(parseCommandLine(u8"  \t  ") == R{});
        }
        SECTION("Unqualified arguments are intepreted as files to be checked") {
            CHECK(parseCommandLine(u8"file1.sfv") ==
                R{ .filesToCheck = { u"file1.sfv" }, .outFile = u"" });
            CHECK(parseCommandLine(u8"c:\\some_foled\\file1.sfv") ==
                R{ .filesToCheck = { u"c:\\some_foled\\file1.sfv" }, .outFile = u"" });
            CHECK(parseCommandLine(u8"c:\\some_foled\\file1.sfv \"c:\\some other folder\\file2.sfv\" c:\\file3.md5") ==
                R{ .filesToCheck = { u"c:\\some_foled\\file1.sfv", u"c:\\some other folder\\file2.sfv", u"c:\\file3.md5" },
                   .outFile = u"" });
        }
        SECTION("DOALL arguments are ignored")
        {
            CHECK(parseCommandLine(u8"DOALL file1.sfv") ==
                R{ .filesToCheck = { u"file1.sfv" }, .outFile = u"" });
            CHECK(parseCommandLine(u8"DOALL c:\\some_foled\\file1.sfv DOALL DOALL") ==
                R{ .filesToCheck = { u"c:\\some_foled\\file1.sfv" }, .outFile = u"" });
        }
        SECTION("OUTPUT argument sets outfile")
        {
            CHECK(parseCommandLine(u8"DOALL file1.sfv OUTPUT:out1.txt") ==
                R{ .filesToCheck = { u"file1.sfv" }, .outFile = u"out1.txt" });
            CHECK(parseCommandLine(u8"OUTPUT:out2.txt c:\\some_foled\\file1.sfv ") ==
                R{ .filesToCheck = { u"c:\\some_foled\\file1.sfv" }, .outFile = u"out2.txt" });
            CHECK(parseCommandLine(u8"OUTPUT:\"c:\\path with spaces\\file 1.txt\" c:\\some_foled\\file1.sfv ") ==
                R{ .filesToCheck = { u"c:\\some_foled\\file1.sfv" }, .outFile = u"c:\\path with spaces\\file 1.txt" });
        }
    }
}
