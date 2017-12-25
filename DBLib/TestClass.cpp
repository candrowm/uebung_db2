#define CATCH_CONFIG_MAIN

#include <hubDB/catch.hpp>
#include <hubDB/DBMyBufferMgr.h>

using namespace HubDB::Manager;

int foo() {
    return 2;
}

TEST_CASE("My First Test") {
    REQUIRE(foo() == 3);
}


