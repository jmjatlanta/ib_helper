#pragma once
#include <string>
#include <cstdint>
#include <map>
#include <vector>
#include <functional>
#include <variant>

class BookRecord
{
    public:
    std::string exchange;
    double price;
    uint32_t size;

    BookRecord(const std::string& exch, double p, uint64_t sz) : exchange(exch), price(p), size(sz) {}
    std::string to_string();
};

class Book
{
    public:
    // these hold the details
    std::vector<BookRecord> bidBookRecords;
    std::vector<BookRecord> askBookRecords;
    // these hold the summary
    std::map<double, uint64_t, std::greater<double> > bidPriceLevels;
    std::map<double, uint64_t, std::less<double> > askPriceLevels;

    /****
     * @param position the position in the book
     * @param marketMaker who is making the bid/offer
     * @param operation (0) insert   (1) update   (2) remove
     * @param side bid / ask
     * @param price
     * @param size
     * @param isSmartDepth
     */
    void update(uint16_t position, const std::string& marketMaker, uint16_t operation, uint16_t side, double price, 
            uint64_t size, bool isSmartDepth);

    /***
     * For a particular depth (record position, not price), get the size
     * @param depth the depth
     * @param side 0 = bid, 1 = ask
     * @return the size at the level `depth`
     */
    uint64_t sizeAtLevel(uint16_t depth, uint16_t side);

    /***
     * For a particular depth (record position, not price), get the price 
     * @param depth the depth
     * @param side 0 = bid, 1 = ask
     * @return the price at the level `depth`
     */
    double priceAtLevel(uint16_t depth, uint16_t side);

    private:
    void add(uint16_t position, const std::string& marketMaker, uint16_t side, double price, uint64_t size, 
            bool isSmartDepth);
    void update(uint16_t position, const std::string& marketMaker, uint16_t side, double price, uint64_t size, 
            bool isSmartDepth);
    void remove(uint16_t position, uint16_t side);
    std::vector<BookRecord>* getBookRecordVector(uint16_t side);
};
