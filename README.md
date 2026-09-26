# OBD-II Reader

A small C++17 command-line application for reading basic engine data through an
ELM327-compatible OBD-II adapter. It communicates with the adapter over a
POSIX serial port, initializes it with AT commands, sends standard OBD-II Mode
01 requests, validates the responses, and prints the decoded values.

The repository also includes a file-backed demo mode, so the application can
be built and tried without a vehicle or an OBD-II adapter.

## Features

- Communication with ELM327-compatible adapters over a serial port
- Automatic adapter initialization and protocol selection
- Engine coolant temperature reading (Mode 01, PID `05`)
- Engine speed reading (Mode 01, PID `0C`)
- Demo mode using predefined responses from `data/demo.txt`
- Input validation and separate connection/protocol error handling
- Automatic cleanup of the serial connection through RAII

## Requirements

- A C++17-compatible compiler such as GCC or Clang
- GNU Make or a compatible `make` implementation
- A POSIX-compatible operating system with `termios` serial-port support, such
  as Linux or macOS
- For live vehicle data:
  - an ELM327-compatible OBD-II adapter with a serial interface
  - an OBD-II-compatible vehicle
  - permission to access the adapter's serial device

No third-party libraries are required.

## Build

Clone the repository, enter its directory, and run:

```sh
make
```

This builds the `build/obd_reader` executable with the following compiler
settings:

```text
-std=c++17 -Wall -Wextra -Wpedantic
```

To remove the executable and any object files, run:

```sh
make clean
```

## Quick Start: Demo Mode

Run the included demonstration without connecting an adapter:

```sh
make run-demo
```

The demo queries both supported values and then exits. With the bundled
`data/demo.txt`, the decoded results are:

```text
Coolant temperature: 50 °C
Engine speed: 1,726 rpm (printed 50 times)
```

The program displays temperature as `C` rather than using the degree symbol.

You can also start demo mode manually:

```sh
./build/obd_reader
```

Enter `data/demo.txt` when prompted for the serial port. A baud-rate choice is
still requested, but it is ignored in demo mode.

## Using a Real Adapter

1. Connect the ELM327-compatible adapter to the vehicle and computer.
2. Turn the vehicle ignition on. Some PIDs, including RPM, require the engine
   to be running.
3. Identify the adapter's serial device. Common examples include:

   ```text
   /dev/ttyUSB0
   /dev/ttyACM0
   /dev/tty.usbserial-0001
   ```

4. Start the application:

   ```sh
   ./build/obd_reader
   ```

5. Enter the serial-device path and select the baud rate used by the adapter:

   | Menu choice | Baud rate |
   | --- | ---: |
   | `1` | 9,600 |
   | `2` | 38,400 |
   | `3` | 115,200 |

   Many ELM327 adapters use 38,400 baud by default, but the correct value
   depends on the adapter.

6. Select an operation from the interactive menu:

   | Menu choice | Operation |
   | --- | --- |
   | `1` | Read engine coolant temperature once |
   | `2` | Read engine speed 50 times in succession |
   | `3` | Exit |

If opening or initializing the connection fails, the application reports the
error and offers the option to enter another serial-port path. Reply with `y`
(yes) to retry or `n` (no) to exit.

### Serial-Port Permissions on Linux

If the device exists but cannot be opened, check its permissions:

```sh
ls -l /dev/ttyUSB0
```

Serial devices are commonly assigned to the `dialout` group. The exact group
and access procedure vary by distribution; avoid running the application as
root unless you understand the security implications.

## Adapter Initialization

For a live connection, the application sends these commands in order:

| Command | Purpose |
| --- | --- |
| `ATZ` | Reset the adapter |
| `ATE0` | Disable command echo |
| `ATL0` | Disable line feeds |
| `ATS0` | Disable spaces in responses |
| `ATH0` | Hide protocol headers |
| `ATSP0` | Select the OBD protocol automatically |

Every command after `ATZ` must return a response containing `OK`; otherwise,
initialization fails.

The serial connection uses 8 data bits, no parity, one stop bit (8N1), no
hardware flow control, raw input/output, and an approximately one-second read
timeout.

## Supported Data

### Engine coolant temperature

- Request: `01 05`
- Expected response prefix: `41 05`
- Data bytes: `A`
- Conversion: `temperature (°C) = A - 40`

For the demo response `41 05 5A`, hexadecimal `5A` is decimal 90, producing
`90 - 40 = 50 °C`.

### Engine speed

- Request: `01 0C`
- Expected response prefix: `41 0C`
- Data bytes: `A B`
- Conversion: `RPM = ((A × 256) + B) / 4`

For the demo response `41 0C 1A F8`, the result is 1,726 rpm. The implementation
uses integer arithmetic, so any fractional result is truncated.

## Demo Data Format

Demo responses are stored as one command/response pair per line:

```text
COMMAND=RESPONSE
```

The bundled file contains:

```text
0105=41 05 5A >
010C=41 0C 1A F8 >
```

Carriage returns, line feeds, spaces, and the ELM327 prompt character (`>`) are
removed before parsing. A demo file is recognized when its path is exactly
`demo.txt` or ends with `/demo.txt`, including the bundled `data/demo.txt`.

## Project Structure

```text
.
├── src/                         C++ implementation files
│   ├── main.cc                  CLI and program entry point
│   ├── ObdConnection.cc         Serial and demo communication
│   ├── ObdReader.cc             PID validation and decoding
│   └── ObdUtils.cc              Shared utility implementations
├── include/obd/                 Public project headers
│   ├── ObdConnection.h
│   ├── ObdException.h
│   ├── ObdReader.h
│   └── ObdUtils.h
├── data/
│   └── demo.txt                 Demo command/response data
├── build/                       Generated objects, dependencies, and executable
├── Makefile                     Build, demo, and cleanup targets
├── README.md                    Project documentation
└── LICENSE                      MIT License
```

## Error Handling

The application detects and reports conditions including:

- unsupported baud rates
- serial-port open or configuration failures
- command write and response read failures
- empty, oversized, or malformed responses
- `NO DATA`, `ERROR`, and `?` adapter responses
- unexpected OBD-II response headers
- missing or invalid hexadecimal data bytes
- missing commands in the demo file

Connection and adapter-initialization errors return to the connection retry
prompt. Errors encountered while reading a PID are reported without terminating
the menu immediately; an RPM error stops the current 50-read sequence.

## Current Limitations

- Only coolant temperature and engine speed are supported.
- The application is interactive; it does not currently accept command-line
  arguments.
- The serial implementation targets POSIX systems and does not support native
  Windows serial ports.
- Responses are handled as a single cleaned text stream. Multi-ECU or complex
  multi-frame responses are not explicitly decoded.
- The adapter must use one of the three supported baud rates.
- There is no automated test suite in the repository.

## Safety

Use the application only when the vehicle is safely parked and ventilated. Do
not operate a computer or change connections while driving. Reading diagnostic
data is generally non-invasive, but adapter quality and vehicle behavior vary.

## License

This project is available under the [MIT License](LICENSE).
