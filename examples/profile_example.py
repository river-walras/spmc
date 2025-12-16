"""
性能分析示例

使用 cProfile 分析性能瓶颈
"""

import msgbus
import time
import random
import cProfile
import pstats
from io import StringIO


def simple_callback(data_type, data):
    """简单回调 - 只访问数据不做任何处理"""
    _ = data['timestamp']
    _ = data['symbol']
    _ = data['price']


def main():
    print("=" * 60)
    print("Performance Profiling - msgbus")
    print("=" * 60)

    hub = msgbus.MarketDataHub()

    # 创建1个订阅者
    print("\n[1] Testing with 1 subscriber...")
    sub1 = hub.subscribe(msgbus.DataType.TRADE, simple_callback)

    profiler = cProfile.Profile()
    profiler.enable()

    start = time.time()
    num_messages = 100000
    base_price = 50000.0

    for i in range(num_messages):
        trade = msgbus.Trade()
        trade.timestamp = time.time_ns()
        trade.symbol = "BTCUSDT"
        trade.price = base_price + random.uniform(-100, 100)
        trade.quantity = random.uniform(0.01, 1.0)
        trade.is_buyer_maker = random.choice([True, False])
        hub.add_trade(trade)

    elapsed = time.time() - start

    profiler.disable()

    # 等待消费完成
    time.sleep(1)

    print(f"Messages sent: {num_messages}")
    print(f"Time elapsed: {elapsed:.2f}s")
    print(f"Throughput: {num_messages / elapsed:.0f} msgs/sec")

    # 打印性能分析结果
    print("\n" + "=" * 60)
    print("Top 20 functions by cumulative time:")
    print("=" * 60)

    s = StringIO()
    ps = pstats.Stats(profiler, stream=s)
    ps.strip_dirs()
    ps.sort_stats('cumulative')
    ps.print_stats(20)
    print(s.getvalue())

    print("\n" + "=" * 60)
    print("Top 20 functions by total time:")
    print("=" * 60)

    s = StringIO()
    ps = pstats.Stats(profiler, stream=s)
    ps.strip_dirs()
    ps.sort_stats('tottime')
    ps.print_stats(20)
    print(s.getvalue())

    hub.stop_all()

    # 测试多订阅者
    print("\n\n" + "=" * 60)
    print("[2] Testing with 4 subscribers...")
    print("=" * 60)

    hub2 = msgbus.MarketDataHub()
    sub1 = hub2.subscribe(msgbus.DataType.TRADE, simple_callback)
    sub2 = hub2.subscribe(msgbus.DataType.TRADE, simple_callback)
    sub3 = hub2.subscribe(msgbus.DataType.TRADE, simple_callback)
    sub4 = hub2.subscribe(msgbus.DataType.TRADE, simple_callback)

    profiler2 = cProfile.Profile()
    profiler2.enable()

    start = time.time()

    for i in range(num_messages):
        trade = msgbus.Trade()
        trade.timestamp = time.time_ns()
        trade.symbol = "BTCUSDT"
        trade.price = base_price + random.uniform(-100, 100)
        trade.quantity = random.uniform(0.01, 1.0)
        trade.is_buyer_maker = random.choice([True, False])
        hub2.add_trade(trade)

    elapsed = time.time() - start

    profiler2.disable()

    # 等待消费完成
    time.sleep(2)

    print(f"Messages sent: {num_messages}")
    print(f"Time elapsed: {elapsed:.2f}s")
    print(f"Throughput: {num_messages / elapsed:.0f} msgs/sec")

    # 打印性能分析结果
    print("\n" + "=" * 60)
    print("Top 20 functions by cumulative time:")
    print("=" * 60)

    s = StringIO()
    ps = pstats.Stats(profiler2, stream=s)
    ps.strip_dirs()
    ps.sort_stats('cumulative')
    ps.print_stats(20)
    print(s.getvalue())

    print("\n" + "=" * 60)
    print("Top 20 functions by total time:")
    print("=" * 60)

    s = StringIO()
    ps = pstats.Stats(profiler2, stream=s)
    ps.strip_dirs()
    ps.sort_stats('tottime')
    ps.print_stats(20)
    print(s.getvalue())

    hub2.stop_all()


if __name__ == "__main__":
    main()
