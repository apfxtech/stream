# Stream

`stream` is a module used in internal projects.
It provides an **Arduino-style** API for **Serial/WebSocket** so the same `Serial`-based code can run on multiple platforms **without code changes**.
The repository also includes **standards and utilities** for low-level byte processing.

## Features

* Unified Arduino-style interface for `Serial` and `WebSocket`.
* Run the same code across platforms without rewriting.
* Byte-level utilities/standards (parsing, packing, conversions, etc.).

## Flipper Serial

On Flipper (`FURI_OS`) serial can be initialized with a simple port number:

```cpp
uSerial stream;
stream.begin(1, 115200); // 1 = USART, 2 = LPUART
```

## Branches

This repository uses two main branches:

* **`main`** — stable code.
* **`dev`** — active development.

## Contributors

<a href="https://github.com/apfxtech/stream/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=apfxtech/stream" />
</a>
