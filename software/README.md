# Project Dynamic Ankle Brace - Software
Source code for Project Dynamic Ankle Brace (FYDP).

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
    export IDF_PATH=<path-to-repo>/software/external/esp/esp-idf
    export IDF_TOOLS_PATH=<path-to-repo>/software/external/esp/.espressif
    export PATH="$IDF_PATH/tools:$PATH"
    ```
    Tip: add the above lines to your `~/.bashrc` or `~/.zshrc` file so that you don't have to re-export the environment variables when you open a new terminal.

4. Set up tools
    ```bash
    ./external/esp/esp-idf/install.sh esp32s3
    ```

5. Run
    ```bash
    . ./external/esp/esp-idf/export.sh
    ```
    Tip: add the following line to your `~/.bashrc` or `~/.zshrc` file so that you don't have to rerun the script when you open a new terminal (**ONLY** do this if you've added the export lines in step 3 above).
    ```bash
    source $IDF_PATH/export.sh > /dev/null
    ```

See [ESP-IDF Programming Guide: Getting Started](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/linux-macos-setup.html#get-started-linux-macos-first-steps) for reference

## Building and Flashing the Example Hello World Project

```bash
cd examples/hello_world
idf.py build
idf.py flash monitor
```

## Clang Format

Make sure C/C++ source files are formatted following the clang format rule. It is recommended to turn on "Format On Save" if developing in VSCode.
