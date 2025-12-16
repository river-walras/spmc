#pragma once

#include <cstdint>
#include <variant>
#include <cstring>

// Market data type definitions
namespace marketdata {

// K线数据
struct Kline {
    uint64_t timestamp;      // 时间戳 (纳秒)
    double open;             // 开盘价
    double high;             // 最高价
    double low;              // 最低价
    double close;            // 收盘价
    double volume;           // 成交量
    char symbol[32];         // 交易对符号

    Kline() : timestamp(0), open(0), high(0), low(0), close(0), volume(0) {
        symbol[0] = '\0';
    }
};

// 逐笔成交数据
struct Trade {
    uint64_t timestamp;      // 时间戳 (纳秒)
    double price;            // 成交价格
    double quantity;         // 成交数量
    char symbol[32];         // 交易对符号
    bool is_buyer_maker;     // 是否买方挂单

    Trade() : timestamp(0), price(0), quantity(0), is_buyer_maker(false) {
        symbol[0] = '\0';
    }
};

// Level1 行情数据
struct BookL1 {
    uint64_t timestamp;      // 时间戳 (纳秒)
    double bid_price;        // 买一价
    double bid_quantity;     // 买一量
    double ask_price;        // 卖一价
    double ask_quantity;     // 卖一量
    char symbol[32];         // 交易对符号

    BookL1() : timestamp(0), bid_price(0), bid_quantity(0),
               ask_price(0), ask_quantity(0) {
        symbol[0] = '\0';
    }
};

// 使用 variant 来统一表示不同类型的市场数据
// variant 是零开销抽象,大小为 max(sizeof(Kline), sizeof(Trade), sizeof(BookL1)) + sizeof(size_t)
using MarketData = std::variant<Kline, Trade, BookL1>;

// 用于区分数据类型的枚举
enum class DataType {
    KLINE = 0,
    TRADE = 1,
    BOOK_L1 = 2
};

} // namespace marketdata
