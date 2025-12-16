"""
C++ 生产者性能测试

对比 Python 生产者 vs C++ 生产者的性能差异
"""

import msgbus
import time
import gc


def empty_callback(data_type, data):
    """空回调 - 最小开销"""
    pass


def test_python_producer(num_messages, num_subscribers):
    """测试 Python 生产者"""
    print(f"\n{'=' * 60}")
    print(f"Python Producer - {num_subscribers} subscribers")
    print(f"{'=' * 60}")

    gc.disable()

    hub = msgbus.MarketDataHub()

    # 创建订阅者
    subs = []
    for _ in range(num_subscribers):
        subs.append(hub.subscribe(msgbus.DataType.TRADE, empty_callback))

    # 预创建所有 trade 对象
    trades = []
    for i in range(num_messages):
        trade = msgbus.Trade()
        trade.timestamp = i
        trade.symbol = "BTCUSDT"
        trade.price = 50000.0 + (i % 100)
        trade.quantity = 1.0
        trade.is_buyer_maker = False
        trades.append(trade)

    # 测量生产性能
    start = time.perf_counter()

    for trade in trades:
        hub.add_trade(trade, True)

    produce_time = time.perf_counter() - start

    # 等待消费完成
    time.sleep(2)

    print(f"Messages: {num_messages}")
    print(f"Production time: {produce_time:.3f}s")
    print(f"Throughput: {num_messages / produce_time:,.0f} msgs/sec")

    hub.stop_all()
    gc.enable()
    gc.collect()

    return produce_time


def test_cpp_producer(num_messages, num_subscribers):
    """测试 C++ 生产者"""
    print(f"\n{'=' * 60}")
    print(f"C++ Producer - {num_subscribers} subscribers")
    print(f"{'=' * 60}")

    gc.disable()

    hub = msgbus.MarketDataHub()

    # 创建订阅者
    subs = []
    for _ in range(num_subscribers):
        subs.append(hub.subscribe(msgbus.DataType.TRADE, empty_callback))

    # 创建 C++ 生产者（注意：需要传递 hub 的地址）
    # pybind11 会自动处理指针传递
    producer = msgbus.MockCppProducer(hub)

    # 测量生产性能
    start = time.perf_counter()

    # 启动 C++ 生产者线程（message_type=0 表示 Trade）
    producer.start(num_messages, message_type=0)

    # 等待生产完成
    producer.wait()

    produce_time = time.perf_counter() - start

    # 等待消费完成
    time.sleep(2)

    print(f"Messages: {num_messages}")
    print(f"Messages produced: {producer.messages_produced()}")
    print(f"Production time: {produce_time:.3f}s")
    print(f"Throughput: {num_messages / produce_time:,.0f} msgs/sec")

    hub.stop_all()
    gc.enable()
    gc.collect()

    return produce_time


def main():
    print("=" * 60)
    print("C++ Producer vs Python Producer Benchmark")
    print("=" * 60)

    num_messages = 100000

    # 测试 1: 无订阅者
    print("\n" + "=" * 60)
    print("Test 1: No subscribers (baseline)")
    print("=" * 60)

    py_time_0 = test_python_producer(num_messages, num_subscribers=0)
    cpp_time_0 = test_cpp_producer(num_messages, num_subscribers=0)

    # 测试 2: 1 个订阅者
    print("\n" + "=" * 60)
    print("Test 2: 1 subscriber")
    print("=" * 60)

    py_time_1 = test_python_producer(num_messages, num_subscribers=1)
    cpp_time_1 = test_cpp_producer(num_messages, num_subscribers=1)

    # 测试 3: 4 个订阅者
    print("\n" + "=" * 60)
    print("Test 3: 4 subscribers")
    print("=" * 60)

    py_time_4 = test_python_producer(num_messages, num_subscribers=4)
    cpp_time_4 = test_cpp_producer(num_messages, num_subscribers=4)

    # 测试 4: 8 个订阅者
    print("\n" + "=" * 60)
    print("Test 4: 8 subscribers")
    print("=" * 60)

    py_time_8 = test_python_producer(num_messages, num_subscribers=8)
    cpp_time_8 = test_cpp_producer(num_messages, num_subscribers=8)

    # 汇总结果
    print("\n\n" + "=" * 60)
    print("SUMMARY - Production Throughput (msgs/sec)")
    print("=" * 60)
    print(f"{'Scenario':<30} {'Python':<15} {'C++':<15} {'Speedup':<10}")
    print("-" * 60)

    print(f"{'0 subscribers':<30} {num_messages/py_time_0:>13,.0f}  {num_messages/cpp_time_0:>13,.0f}  {py_time_0/cpp_time_0:>8.1f}x")
    print(f"{'1 subscriber':<30} {num_messages/py_time_1:>13,.0f}  {num_messages/cpp_time_1:>13,.0f}  {py_time_1/cpp_time_1:>8.1f}x")
    print(f"{'4 subscribers':<30} {num_messages/py_time_4:>13,.0f}  {num_messages/cpp_time_4:>13,.0f}  {py_time_4/cpp_time_4:>8.1f}x")
    print(f"{'8 subscribers':<30} {num_messages/py_time_8:>13,.0f}  {num_messages/cpp_time_8:>13,.0f}  {py_time_8/cpp_time_8:>8.1f}x")

    print("\n" + "=" * 60)
    print("KEY INSIGHTS")
    print("=" * 60)

    speedup_0 = py_time_0 / cpp_time_0
    speedup_1 = py_time_1 / cpp_time_1
    speedup_4 = py_time_4 / cpp_time_4
    speedup_8 = py_time_8 / cpp_time_8

    print(f"1. C++ producer is {speedup_0:.1f}x faster with no subscribers")
    print(f"2. C++ producer is {speedup_1:.1f}x faster with 1 subscriber")
    print(f"3. C++ producer is {speedup_4:.1f}x faster with 4 subscribers")
    print(f"4. C++ producer is {speedup_8:.1f}x faster with 8 subscribers")
    print(f"\n5. C++ producer eliminates Python GIL overhead on production side")
    print(f"6. Consumer GIL contention still exists (same for both)")

    if speedup_8 > speedup_4 > speedup_1:
        print(f"\n7. ⚠️  With more subscribers, C++ advantage INCREASES dramatically!")
        print(f"   Python producer competes with ALL consumers for the same GIL")
        print(f"   C++ producer runs freely without GIL contention")

        # 计算 Python 生产者的性能衰减
        py_degradation_4 = (py_time_4 / py_time_1)
        py_degradation_8 = (py_time_8 / py_time_1)
        print(f"\n8. Python producer performance degradation:")
        print(f"   1→4 subscribers: {py_degradation_4:.1f}x slower")
        print(f"   1→8 subscribers: {py_degradation_8:.1f}x slower")

        # 计算 C++ 生产者的稳定性
        cpp_stability_4 = cpp_time_4 / cpp_time_0
        cpp_stability_8 = cpp_time_8 / cpp_time_0
        print(f"\n9. C++ producer remains stable:")
        print(f"   0→4 subscribers: {cpp_stability_4:.2f}x (minimal impact)")
        print(f"   0→8 subscribers: {cpp_stability_8:.2f}x (minimal impact)")
    else:
        print(f"\n7. Speedup is consistent across subscriber counts")


if __name__ == "__main__":
    main()
