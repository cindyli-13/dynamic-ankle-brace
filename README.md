# Project Dynamic Ankle Brace
Source code for Project Dynamic Ankle Brace (FYDP)

## Getting Started

1. Install prereqs

    Ubuntu and Debian:
    ```bash
    sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
    ```
    MacOS:
    ```bash
    brew install cmake ninja dfu-util
    ```

2. Initialize submodules:
    ```bash
    git submodule update --init --recursive
    ```

3. Set the environment variables `IDF_PATH` and `IDF_TOOLS_PATH` and update `PATH`:
    ```bash
    export IDF_PATH=<path-to-repo>/external/esp/esp-idf
    export IDF_TOOLS_PATH=<path-to-repo>/external/esp/.espressif
    export PATH="$IDF_PATH/tools:$PATH"
    ```

4. Set up tools
    ```bash
    ./external/esp/esp-idf/install.sh esp32s3
    ```

5. Run
    ```bash
    . ./external/esp/esp-idf/export.sh
    ```

See [ESP-IDF Programming Guide: Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/linux-macos-setup.html#get-started-linux-macos-first-steps) for reference

## Building and Flashing the Example Hello World Project

```bash
cd examples/hello_world
idf.py build
idf.py -p <PORT> flash
```
