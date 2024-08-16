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
make
make install
```
