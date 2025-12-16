"""
msgbus - High-performance SPMC (Single Producer Multiple Consumer) Market Data Hub

A lock-free, wait-free message bus for distributing market data with ultra-low latency.
Built with C++ and exposed to Python via pybind11.
"""

try:
    from ._core import (
        DataType,
        Kline,
        Trade,
        BookL1,
        MarketDataHub,
        MockCppProducer,
    )
except ImportError as e:
    raise ImportError(
        f"Failed to import msgbus C++ extension module: {e}\n"
        "Make sure the package is properly installed."
    ) from e

__version__ = "0.1.0"

__all__ = [
    "DataType",
    "Kline",
    "Trade",
    "BookL1",
    "MarketDataHub",
    "MockCppProducer",
]
