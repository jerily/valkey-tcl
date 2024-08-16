# valkey-tcl

TCL/C extension that provides an interface to the Valkey key-value store.

## Requirements

- [libvalkey](https://github.com/valkey-io/libvalkey)

## Install Dependencies

```bash
git clone https://github.com/valkey-io/libvalkey.git
cd libvalkey
mkdir build
cd build
cmake .. -DENABLE_SSL=OFF
make
make install
```

## Install valkey-tcl

```bash
git clone https://github.com/jerily/valkey-tcl.git
cd valkey-tcl
mkdir build
cd build
cmake ..
# or if TCL is not in the default path (/usr/local/lib):
# change "TCL_LIBRARY_DIR" and "TCL_INCLUDE_DIR" to the correct paths
# cmake .. -DTCL_LIBRARY_DIR=/usr/local/lib -DTCL_INCLUDE_DIR=/usr/local/include
make
make install
```
