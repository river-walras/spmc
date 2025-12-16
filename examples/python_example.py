"""
Python示例: 使用SPMC Market Data Hub

这个示例展示如何:
1. 创建 MarketDataHub 实例
2. 订阅不同类型的市场数据 (Kline, Trade, BookL1)
3. 从 Python 生产者添加数据
4. C++ 后台线程自动分发数据到订阅者的回调函数
"""

import msgbus
import time
import random


# 定义回调函数
def kline_callback(data_type, data):
    """K线数据回调"""
    print(f"[KLINE] {data['symbol']}: O={data['open']:.2f}, H={data['high']:.2f}, "
          f"L={data['low']:.2f}, C={data['close']:.2f}, V={data['volume']:.2f}")


def trade_callback(data_type, data):
    """成交数据回调"""
    side = "SELL" if data['is_buyer_maker'] else "BUY"
    print(f"[TRADE] {data['symbol']}: {side} {data['quantity']:.4f} @ {data['price']:.2f}")


def book_l1_callback(data_type, data):
    """L1行情回调"""
    print(f"[BOOK] {data['symbol']}: Bid={data['bid_price']:.2f}x{data['bid_quantity']:.4f}, "
          f"Ask={data['ask_price']:.2f}x{data['ask_quantity']:.4f}")


def main():
    # 创建 Hub
    hub = msgbus.MarketDataHub()
    print("MarketDataHub created")

    # 订阅不同类型的数据
    kline_sub_id = hub.subscribe(msgbus.DataType.KLINE, kline_callback)
    trade_sub_id = hub.subscribe(msgbus.DataType.TRADE, trade_callback)
    book_sub_id = hub.subscribe(msgbus.DataType.BOOK_L1, book_l1_callback)

    print(f"Subscribed to 3 data types")
    print(f"Active subscribers: {hub.subscriber_count()}\n")

    # 模拟生产数据
    base_price = 50000.0

    try:
        for i in range(20):
            # 模拟价格变化
            price = base_price + random.uniform(-100, 100)

            # 每次循环添加不同类型的数据
            if i % 3 == 0:
                # 添加 K线数据
                kline = msgbus.Kline()
                kline.timestamp = int(time.time() * 1e9)
                kline.symbol = "BTCUSDT"
                kline.open = price
                kline.high = price + random.uniform(0, 50)
                kline.low = price - random.uniform(0, 50)
                kline.close = price + random.uniform(-20, 20)
                kline.volume = random.uniform(1, 10)
                hub.add_kline(kline, True)

            elif i % 3 == 1:
                # 添加 Trade 数据
                trade = msgbus.Trade()
                trade.timestamp = int(time.time() * 1e9)
                trade.symbol = "BTCUSDT"
                trade.price = price
                trade.quantity = random.uniform(0.01, 1.0)
                trade.is_buyer_maker = random.choice([True, False])
                hub.add_trade(trade, True)

            else:
                # 添加 BookL1 数据
                book = msgbus.BookL1()
                book.timestamp = int(time.time() * 1e9)
                book.symbol = "BTCUSDT"
                book.bid_price = price - 0.5
                book.bid_quantity = random.uniform(1, 10)
                book.ask_price = price + 0.5
                book.ask_quantity = random.uniform(1, 10)
                hub.add_book_l1(book, True)

            time.sleep(0.1)  # 模拟数据间隔

    except KeyboardInterrupt:
        print("\nStopping...")

    # 清理
    print("\nCleaning up...")
    hub.stop_all()
    print("All subscribers stopped")


if __name__ == "__main__":
    main()
