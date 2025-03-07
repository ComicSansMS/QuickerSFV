#include <quicker_sfv/line_reader.hpp>

#include <quicker_sfv/error.hpp>
#include <quicker_sfv/file_io.hpp>

#include <test_file_io.hpp>

#include <cstring>
#include <ranges>
#include <utility>

#include <catch.hpp>

TEST_CASE("Line Reader")
{
    using quicker_sfv::LineReader;
    
    TestInput input;
    LineReader r{ input };
    std::optional<std::u8string> line;

    SECTION("Read from empty file") {
        input = "";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->empty());
        CHECK(r.done());

        SECTION("Reading from done reader gives no result") {
            line = r.read_line();
            CHECK(!line);
        }
    }
    SECTION("Read from file with no newline")
    {
        input = "Hello! this is a single string with no linebreaks";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"Hello! this is a single string with no linebreaks");
        CHECK(r.done());
        line = r.read_line();
    }
    SECTION("Single newline")
    {
        input = "\n";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->empty());
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->empty());
        CHECK(r.done());
    }
    SECTION("Line breaks of different kinds")
    {
        input = "A1\nB1\rC1\r\nD1";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"A1");
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"B1\rC1");
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"D1");
        CHECK(r.done());
    }
    SECTION("Empty lines after linebreak")
    {
        input = "Hey\n\nHey again\n";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"Hey");
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"");
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"Hey again");
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"");
        CHECK(r.done());
    }
    auto repeat = [](auto c, std::size_t n) {
        std::basic_string<decltype(c)> ret(n, c);
        return ret;
    };
    SECTION("Line size exceeding buffer size") {
        input = repeat('A', LineReader::READ_BUFFER_SIZE + 10) + "\nBBB";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->size() == LineReader::READ_BUFFER_SIZE + 10);
        CHECK(std::ranges::all_of(*line, [](char8_t c) { return c == u8'A'; }));
        
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"BBB");
        CHECK(r.done());
    }
    SECTION("Newline at buffer boundaries") {
        char const* newlines[] = { "\n", "\r\n" };
        for (char const* nl : newlines) {
            for (int i = -3; i <= 3; ++i) {
                input = repeat('A', LineReader::READ_BUFFER_SIZE + i) + nl + "BBB";
                CHECK(!r.done());
                line = r.read_line();
                REQUIRE(line);
                CHECK(line->size() == LineReader::READ_BUFFER_SIZE + i);
                CHECK(std::ranges::all_of(*line, [](char8_t c) { return c == u8'A'; }));

                CHECK(!r.done());
                line = r.read_line();
                REQUIRE(line);
                CHECK(*line == u8"BBB");
                CHECK(r.done());
                input.read_idx = 0;
                r = LineReader(input);
            }
        }
    }
    SECTION("Valid file spanning multiple buffers") {
        input = repeat('A', LineReader::READ_BUFFER_SIZE - 4) + "\n" +
            repeat('B', LineReader::READ_BUFFER_SIZE - 3) + "\n" +
            repeat('C', LineReader::READ_BUFFER_SIZE - 5) + "\n" +
            repeat('D', LineReader::READ_BUFFER_SIZE) + "\n";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->size() == LineReader::READ_BUFFER_SIZE - 4);
        CHECK(std::ranges::all_of(*line, [](char8_t c) { return c == u8'A'; }));
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->size() == LineReader::READ_BUFFER_SIZE - 3);
        CHECK(std::ranges::all_of(*line, [](char8_t c) { return c == u8'B'; }));
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->size() == LineReader::READ_BUFFER_SIZE - 5);
        CHECK(std::ranges::all_of(*line, [](char8_t c) { return c == u8'C'; }));
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->size() == LineReader::READ_BUFFER_SIZE);
        CHECK(std::ranges::all_of(*line, [](char8_t c) { return c == u8'D'; }));
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->empty());
        CHECK(r.done());
    }
    SECTION("File I/O fault in first read")
    {
        input = "Hey\nHow are you?\n";
        input.fault_after = 10;
        CHECK(!r.done());
        CHECK_THROWS_AS(r.read_line(), quicker_sfv::Exception);
        CHECK(input.read_calls == 1);
    }
    SECTION("File I/O fault in the second read")
    {
        input = repeat('A', LineReader::READ_BUFFER_SIZE - 3) + "\nBBBBBB";
        input.fault_after = LineReader::READ_BUFFER_SIZE + 1;
        CHECK(!r.done());
        CHECK_THROWS_AS(r.read_line(), quicker_sfv::Exception);
        CHECK(input.read_calls == 2);
    }
    SECTION("File I/O fault in the third read")
    {
        input = repeat('A', 2 * LineReader::READ_BUFFER_SIZE - 3) + "\nBBBBBB";
        input.fault_after = 2 * LineReader::READ_BUFFER_SIZE + 1;
        CHECK(!r.done());
        CHECK_THROWS_AS(r.read_line(), quicker_sfv::Exception);
        CHECK(input.read_calls == 3);
    }
    SECTION("File I/O fault in the fourth read")
    { 
        input = repeat('A', 2 * LineReader::READ_BUFFER_SIZE - 3) + "\n" +
            repeat('B', LineReader::READ_BUFFER_SIZE) + "\n" +
            repeat('C', LineReader::READ_BUFFER_SIZE) + "\n" +
            repeat('D', LineReader::READ_BUFFER_SIZE) + "\n" +
            "\nEEEEEE";
        input.fault_after = 3 * LineReader::READ_BUFFER_SIZE + 1;
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->size() == 2 * LineReader::READ_BUFFER_SIZE - 3);
        CHECK(std::ranges::all_of(*line, [](char c) { return c == 'A'; }));
        CHECK_THROWS_AS(r.read_line(), quicker_sfv::Exception);
        CHECK(input.read_calls == 4);
    }
    SECTION("Line longer than single buffer may not be read") {
        input = repeat('A', 2 * LineReader::READ_BUFFER_SIZE - 3) + "\n" +
            repeat('B', LineReader::READ_BUFFER_SIZE + 20) + "\n";
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(line->size() == 2 * LineReader::READ_BUFFER_SIZE - 3);
        CHECK(std::ranges::all_of(*line, [](char c) { return c == 'A'; }));
        CHECK(!r.done());
        line = r.read_line();
        CHECK(!line);
    }
    SECTION("Invalid UTF-8 in line") {
        input = "AAAAAA\nBB";
        input.contents.push_back(static_cast<char>(128));
        input.contents.push_back('B');
        input.contents.push_back('\n');
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"AAAAAA");
        CHECK(!r.done());
        CHECK_THROWS_AS(r.read_line(), quicker_sfv::Exception);
    }
    SECTION("Invalid UTF-8 in line spanning buffers") {
        input = "AAAAAA\n" + repeat('B', LineReader::READ_BUFFER_SIZE);
        input.contents.push_back(static_cast<char>(128));
        input.contents.push_back('B');
        input.contents.push_back('\n');
        CHECK(!r.done());
        line = r.read_line();
        REQUIRE(line);
        CHECK(*line == u8"AAAAAA");
        CHECK(!r.done());
        CHECK_THROWS_AS(r.read_line(), quicker_sfv::Exception);
    }
}
