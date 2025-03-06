#include <quicker_sfv/error.hpp>

#include <catch.hpp>

TEST_CASE("Error")
{
    using quicker_sfv::Error;
    using quicker_sfv::Exception;
    bool did_catch = false;
    try {
        quicker_sfv::throwException(Error::Failed);
    } catch (std::exception& e) {
        CHECK(dynamic_cast<Exception*>(&e) != nullptr);
        did_catch = true;
    }
    CHECK(did_catch);

    did_catch = false;
    try {
        quicker_sfv::throwException(Error::Failed);
        CHECK(false);
    } catch (quicker_sfv::Exception& e) {
        CHECK(e.code() == Error::Failed);
        CHECK(e.what8() == u8"Failed");
        did_catch = true;
    }
    CHECK(did_catch);

    auto getErrorString = [](Error e)->std::u8string {
        try { throwException(e); } catch (Exception const& e) { return std::u8string{ e.what8() }; }
    };
    CHECK(getErrorString(Error::Failed)         == u8"Failed");
    CHECK(getErrorString(Error::SystemError)    == u8"System error");
    CHECK(getErrorString(Error::FileIO)         == u8"File i/o error");
    CHECK(getErrorString(Error::HasherFailure)  == u8"Failed to hash");
    CHECK(getErrorString(Error::ParserError)    == u8"Invalid file format");
    CHECK(getErrorString(Error::PluginError)    == u8"Plugin failed");

}
