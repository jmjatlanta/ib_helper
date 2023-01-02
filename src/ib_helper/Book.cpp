#include "Book.hpp"
#include "../util/SysLogger.h"
#include <exception>

const std::string clazz("Book");

void Book::update(uint16_t position, const std::string& marketMaker, uint16_t operation, uint16_t side, double price,
        uint64_t size, bool isSmartDepth)
{
    switch(operation)
    {
        case 0: // add
        add(position, marketMaker, side, price, size, isSmartDepth);
        break;
        case 1: // update
        update(position, marketMaker, side, price, size, isSmartDepth);
        break;
        case 2: // remove
        remove(position, side);
        break;
    }
}

/****
 * Get the size at a particular depth
 * @param map the bid or ask map
 * @param depth the number of levels down NOTE: not necessarily number of ticks
 * @returns the size at a particular depth
 */
template<class CMP>
uint64_t szAtLevel(std::map<double, uint64_t, CMP>& map, uint16_t depth)
{
    // get to the right level
    auto itr = map.begin();
    if (map.size() <= depth)
        itr = map.end();
    else
        itr = std::next(itr, depth);
    if (itr == map.end())
        return 0;
    return (*itr).second;
}

uint64_t Book::sizeAtLevel(uint16_t depth, uint16_t side)
{
    if (side == 0)
        return szAtLevel(bidPriceLevels, depth);
    return szAtLevel(askPriceLevels, depth);
}

template<class CMP>
double prcAtLevel(std::map<double, uint64_t, CMP>& map, uint16_t depth)
{
    // get to the right level
    auto itr = map.begin();
    if (map.size() <= depth)
        itr = map.end();
    else
        itr = std::next(itr, depth);
    if (itr == map.end())
        return 0.0;
    return (*itr).first;
}

double Book::priceAtLevel(uint16_t depth, uint16_t side)
{
    if (side == 0)
        return prcAtLevel(bidPriceLevels, depth);
    return prcAtLevel(askPriceLevels, depth);
}

std::vector<BookRecord>* Book::getBookRecordVector(uint16_t side)
{
    if (side == 0)
        return &bidBookRecords;
    return &askBookRecords;
}

void Book::add(uint16_t position, const std::string& marketMaker, uint16_t side, double price, uint64_t size, 
        bool isSmartDepth)
{
    std::vector<BookRecord>* rec = getBookRecordVector(side);
    // insert into vector
    auto itr = rec->begin();
    if (rec->size() <= position)
        itr = rec->end();
    else
        itr = std::next(itr, position);
    rec->insert(itr,  BookRecord{marketMaker, price, size} );
    // adjust size at level
    if (side == 0)
        bidPriceLevels[price] += size;
    else
        askPriceLevels[price] += size;
}

void Book::update(uint16_t position, const std::string& marketMaker, uint16_t side, double price, uint64_t size, 
        bool isSmartDepth)
{
    std::vector<BookRecord>* rec = getBookRecordVector(side);
    // update element in vector
    BookRecord& thisRecord = (*rec)[position];
    uint64_t sizeDelta = thisRecord.size - size;
    thisRecord.size = size;
    // adjust size at level
    if (side == 0)
        bidPriceLevels[price] += sizeDelta;
    else
        askPriceLevels[price] += sizeDelta;
}

void Book::remove(uint16_t position, uint16_t side)
{
    std::vector<BookRecord>* rec = getBookRecordVector(side);
    BookRecord& thisRecord = (*rec)[position];
    double price = thisRecord.price;
    uint64_t sizeRemoved = thisRecord.size;
    auto itr = rec->begin();
    itr = std::next(itr, position);
    rec->erase(itr);
    if (side == 0)
        bidPriceLevels[price] -= sizeRemoved;
    else
        askPriceLevels[price] -= sizeRemoved;
}

std::string BookRecord::to_string()
{
    return exchange + " " + std::to_string(size) + " " + std::to_string(price);
}

