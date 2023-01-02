#include "../src/ib_helper/Book.hpp"
#include "../src/util/SysLogger.h"

#include "gtest/gtest.h"

#include <thread>
#include <chrono>

TEST(BookTest, AddUpdateDelete)
{
    Book book;
    //book.update(int position, const int &marketMaker, int operation, int side, double price, int size, bool isSmartDepth)
    book.update(0, "EDGE", 0, 0, 123.45, 100, true);
    book.update(0, "EDGE", 0, 1, 123.46, 100, true);

    EXPECT_EQ(book.priceAtLevel(0, 0), 123.45);
    EXPECT_EQ(book.sizeAtLevel(0,0), 100);
    EXPECT_EQ(book.priceAtLevel(0,1), 123.46);
    EXPECT_EQ(book.sizeAtLevel(0,1), 100);

    // add to existing level
    book.update(0, "NASD", 0, 0, 123.45, 100, true);
    EXPECT_EQ(book.priceAtLevel(0, 0), 123.45);
    EXPECT_EQ(book.sizeAtLevel(0,0), 200);
    EXPECT_EQ(book.priceAtLevel(0,1), 123.46);
    EXPECT_EQ(book.sizeAtLevel(0,1), 100);
    // that should make EDGE be in the second position
    
    // add a new level below
    book.update(2, "EDGE", 0, 0, 123.44, 100, true);
    EXPECT_EQ(book.priceAtLevel(0, 0), 123.45);
    EXPECT_EQ(book.sizeAtLevel(0,0), 200);
    EXPECT_EQ(book.priceAtLevel(1, 0), 123.44);
    EXPECT_EQ(book.sizeAtLevel(1,0), 100);
    EXPECT_EQ(book.priceAtLevel(0,1), 123.46);
    EXPECT_EQ(book.sizeAtLevel(0,1), 100);
        
    // remove from existing level
    book.update(0, "NASD", 2, 0, 123.45, 100, true);
    EXPECT_EQ(book.priceAtLevel(0, 0), 123.45);
    EXPECT_EQ(book.sizeAtLevel(0,0), 100);
    EXPECT_EQ(book.priceAtLevel(0,1), 123.46);
    EXPECT_EQ(book.sizeAtLevel(0,1), 100);

}
