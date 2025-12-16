"""
吞吐量测试 - 对比不同场景的性能

测试场景：
1. 无回调（仅生产）
2. 1个订阅者（空回调）
3. 4个订阅者（空回调）
4. 4个订阅者（带计算的回调）
"""

import msgbus
import time
import gc


# 空回调 - 什么都不做
def empty_callback(data_type, data):
    pass


# 简单回调 - 只访问字段
def simple_callback(data_type, data):
    _ = data['timestamp']
    _ = data['price']
    _ = data['symbol']


# 复杂回调 - 带计算
counter = {'count': 0, 'sum': 0.0}
def complex_callback(data_type, data):
    counter['count'] += 1
    counter['sum'] += data['price']


def benchmark(name, num_subscribers, callback_fn, num_messages=100000):
    """运行单个 benchmark"""
    print(f"\n{'=' * 60}")
    print(f"{name}")
    print(f"{'=' * 60}")

    # 禁用GC避免干扰
    gc.disable()

    hub = msgbus.MarketDataHub()

    # 创建订阅者
    subs = []
    for _ in range(num_subscribers):
        if callback_fn:
            subs.append(hub.subscribe(msgbus.DataType.TRADE, callback_fn))

    print(f"Subscribers: {num_subscribers}")
    print(f"Messages: {num_messages}")

    # 预创建所有 trade 对象以避免创建开销
    trades = []
    for i in range(num_messages):
        trade = msgbus.Trade()
        trade.timestamp = i
        trade.symbol = "BTCUSDT"
        trade.price = 50000.0 + (i % 100)
        trade.quantity = 1.0
        trade.is_buyer_maker = False
        trades.append(trade)

    # 测量纯生产性能
    start = time.perf_counter()

    for trade in trades:
        hub.add_trade(trade)

    produce_time = time.perf_counter() - start

    # 等待消费完成
    if num_subscribers > 0:
        time.sleep(2)

    print(f"\nProduction time: {produce_time:.3f}s")
    print(f"Production throughput: {num_messages / produce_time:.0f} msgs/sec")

    if num_subscribers > 0:
        total_time = produce_time + 2.0
        print(f"Total time (with 2s wait): {total_time:.3f}s")
        print(f"Overall throughput: {num_messages / total_time:.0f} msgs/sec")

    hub.stop_all()

    gc.enable()
    gc.collect()

    return produce_time


def main():
    print("=" * 60)
    print("msgbus Throughput Benchmark")
    print("=" * 60)

    num_messages = 100000

    # 测试1: 无订阅者（基准性能）
    t1 = benchmark(
        "Test 1: No subscribers (baseline)",
        num_subscribers=0,
        callback_fn=None,
        num_messages=num_messages
    )

    # 测试2: 1个订阅者，空回调
    t2 = benchmark(
        "Test 2: 1 subscriber with empty callback",
        num_subscribers=1,
        callback_fn=empty_callback,
        num_messages=num_messages
    )

    # 测试3: 4个订阅者，空回调
    t3 = benchmark(
        "Test 3: 4 subscribers with empty callback",
        num_subscribers=4,
        callback_fn=empty_callback,
        num_messages=num_messages
    )

    # 测试4: 1个订阅者，简单回调
    t4 = benchmark(
        "Test 4: 1 subscriber with simple callback",
        num_subscribers=1,
        callback_fn=simple_callback,
        num_messages=num_messages
    )

    # 测试5: 4个订阅者，简单回调
    t5 = benchmark(
        "Test 5: 4 subscribers with simple callback",
        num_subscribers=4,
        callback_fn=simple_callback,
        num_messages=num_messages
    )

    # 测试6: 4个订阅者，复杂回调
    counter['count'] = 0
    counter['sum'] = 0.0
    t6 = benchmark(
        "Test 6: 4 subscribers with complex callback",
        num_subscribers=4,
        callback_fn=complex_callback,
        num_messages=num_messages
    )

    # 汇总
    print("\n\n" + "=" * 60)
    print("SUMMARY")
    print("=" * 60)
    print(f"Baseline (no subscribers):          {num_messages/t1:>10.0f} msgs/sec")
    print(f"1 subscriber (empty):               {num_messages/t2:>10.0f} msgs/sec  ({t2/t1:.2f}x)")
    print(f"4 subscribers (empty):              {num_messages/t3:>10.0f} msgs/sec  ({t3/t1:.2f}x)")
    print(f"1 subscriber (simple):              {num_messages/t4:>10.0f} msgs/sec  ({t4/t1:.2f}x)")
    print(f"4 subscribers (simple):             {num_messages/t5:>10.0f} msgs/sec  ({t5/t1:.2f}x)")
    print(f"4 subscribers (complex):            {num_messages/t6:>10.0f} msgs/sec  ({t6/t1:.2f}x)")

    print("\n" + "=" * 60)
    print("OVERHEAD ANALYSIS")
    print("=" * 60)
    overhead_1sub = ((t2 - t1) / t1) * 100
    overhead_4sub = ((t3 - t1) / t1) * 100
    print(f"Adding 1 subscriber overhead:       {overhead_1sub:>6.1f}%")
    print(f"Adding 4 subscribers overhead:      {overhead_4sub:>6.1f}%")
    print(f"Per-subscriber overhead (4 subs):   {overhead_4sub/4:>6.1f}%")

    if t3 > t2:
        gil_factor = (t3 - t1) / (t2 - t1)
        print(f"\nGIL contention factor (4 vs 1):     {gil_factor:>6.2f}x")


if __name__ == "__main__":
    main()
