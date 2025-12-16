"""
高级示例: 多订阅者 + 统计分析

展示:
1. 多个订阅者订阅相同类型数据
2. 订阅者进行实时统计分析
3. 测量延迟性能
"""

import msgbus
import time
import random
import threading
from collections import defaultdict
from statistics import mean, stdev


class LatencyTracker:
    """延迟跟踪器"""
    def __init__(self, name):
        self.name = name
        self.latencies = []
        self.lock = threading.Lock()

    def add(self, latency_ns):
        with self.lock:
            self.latencies.append(latency_ns)

    def print_stats(self):
        with self.lock:
            if not self.latencies:
                print(f"{self.name}: No data")
                return

            latencies_us = [lat / 1000 for lat in self.latencies]  # 转换为微秒
            print(f"\n{self.name} Statistics:")
            print(f"  Count: {len(latencies_us)}")
            print(f"  Mean: {mean(latencies_us):.2f} μs")
            if len(latencies_us) > 1:
                print(f"  StdDev: {stdev(latencies_us):.2f} μs")
            print(f"  Min: {min(latencies_us):.2f} μs")
            print(f"  Max: {max(latencies_us):.2f} μs")

            # 百分位数
            sorted_lat = sorted(latencies_us)
            n = len(sorted_lat)
            print(f"  P50: {sorted_lat[n * 50 // 100]:.2f} μs")
            print(f"  P90: {sorted_lat[n * 90 // 100]:.2f} μs")
            print(f"  P99: {sorted_lat[n * 99 // 100]:.2f} μs")


class PriceMonitor:
    """价格监控器"""
    def __init__(self, name):
        self.name = name
        self.prices = []
        self.lock = threading.Lock()

    def add_price(self, price):
        with self.lock:
            self.prices.append(price)

    def print_summary(self):
        with self.lock:
            if not self.prices:
                return
            print(f"\n{self.name} Price Summary:")
            print(f"  Samples: {len(self.prices)}")
            print(f"  Min: {min(self.prices):.2f}")
            print(f"  Max: {max(self.prices):.2f}")
            print(f"  Mean: {mean(self.prices):.2f}")


def main():
    hub = msgbus.MarketDataHub()

    # 创建多个跟踪器
    tracker1 = LatencyTracker("Trade Subscriber 1")
    tracker2 = LatencyTracker("Trade Subscriber 2")
    tracker3 = LatencyTracker("Trade Subscriber 3")
    tracker4 = LatencyTracker("Trade Subscriber 4")
    price_monitor = PriceMonitor("Trade Price")

    # 订阅者1: 延迟跟踪
    def trade_callback_1(data_type, data):
        now = time.time_ns()
        latency = now - data['timestamp']
        tracker1.add(latency)

    # 订阅者2: 延迟跟踪 + 价格监控
    def trade_callback_2(data_type, data):
        now = time.time_ns()
        latency = now - data['timestamp']
        tracker2.add(latency)
        price_monitor.add_price(data['price'])
    
    def trade_callback_3(data_type, data):
        now = time.time_ns()
        latency = now - data['timestamp']
        tracker3.add(latency)
        price_monitor.add_price(data['price'])
        # 这里可以添加更多的处理逻辑
    
    def trade_callback_4(data_type, data):
        now = time.time_ns()
        latency = now - data['timestamp']
        tracker4.add(latency)
        price_monitor.add_price(data['price'])

    # 订阅者3: K线数据简单打印
    kline_count = {'count': 0}
    def kline_callback(data_type, data):
        kline_count['count'] += 1
        if kline_count['count'] % 100 == 0:
            print(f"Received {kline_count['count']} klines")

    # 创建订阅
    print("Creating subscriptions...")
    sub1 = hub.subscribe(msgbus.DataType.TRADE, trade_callback_1)
    sub2 = hub.subscribe(msgbus.DataType.TRADE, trade_callback_2)
    sub3 = hub.subscribe(msgbus.DataType.TRADE, trade_callback_3)
    sub4 = hub.subscribe(msgbus.DataType.TRADE, trade_callback_4)
    sub5 = hub.subscribe(msgbus.DataType.KLINE, kline_callback)

    print(f"Active subscribers: {hub.subscriber_count()}")
    print("Starting data production...\n")

    # 生产数据
    base_price = 50000.0
    num_trades = 1000000
    num_klines = 1000

    start_time = time.time()

    # 生产 Trade 数据
    for i in range(num_trades):
        trade = msgbus.Trade()
        trade.timestamp = time.time_ns()
        trade.symbol = "BTCUSDT"
        trade.price = base_price + random.uniform(-100, 100)
        trade.quantity = random.uniform(0.01, 1.0)
        trade.is_buyer_maker = random.choice([True, False])
        hub.add_trade(trade, True)

        # 偶尔添加 K线数据
        if i % 10 == 0:
            kline = msgbus.Kline()
            kline.timestamp = time.time_ns()
            kline.symbol = "BTCUSDT"
            kline.open = trade.price
            kline.high = trade.price + random.uniform(0, 50)
            kline.low = trade.price - random.uniform(0, 50)
            kline.close = trade.price + random.uniform(-20, 20)
            kline.volume = random.uniform(1, 10)
            hub.add_kline(kline, True)

    elapsed = time.time() - start_time
    print(f"\nProduction completed in {elapsed:.2f} seconds")
    print(f"Throughput: {num_trades / elapsed:.0f} trades/sec")

    # 等待消费完成
    print("\nWaiting for consumers to process all data...")
    time.sleep(2)

    # 打印统计信息
    print("\n" + "=" * 60)
    tracker1.print_stats()
    tracker2.print_stats()
    tracker3.print_stats()
    tracker4.print_stats()
    price_monitor.print_summary()
    print(f"\nTotal klines received: {kline_count['count']}")

    # 清理
    print("\n" + "=" * 60)
    print("Cleaning up...")
    hub.stop_all()
    print(f"Subscribers remaining: {hub.subscriber_count()}")


if __name__ == "__main__":
    main()
