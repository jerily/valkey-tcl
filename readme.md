# valkey-tcl

TCL/C extension that provides an interface to the Valkey key-value store.

## Requirements

- [libvalkey](https://github.com/valkey-io/libvalkey)

Supported systems:

- Linux
- MacOS

## Installation

### Install Dependencies

```bash
git clone https://github.com/valkey-io/libvalkey.git
cd libvalkey
mkdir build
cd build
cmake ..
make
make install
```

### Install valkey-tcl

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

### Note about SSL support

By default, the libvalkey library and the valkey-tcl package are built without SSL support.

To enable support for SSL connections, the `-DENABLE_SSL=ON` parameter must be added to the cmake configuration step. This should be done when building libvalkey as well as when building valkey-tcl:

```bash
cmake .. -DENABLE_SSL=ON
make
make install
```

For a successful build with support for SSL connections, the developer packages from OpenSSL must be installed in the system. For example, for the Ubuntu distribution, developer packages can be installed using this command:

```bash
sudo apt-get install -y libssl-dev
```

## Usage

### Establishing a connection

This package defines a single `valkey` command. This command creates a connection (valkey context) that will be used to send commands to a remote server.

This command has the following parameters:

* **-path path** - path to UNIX socket
* **-host hostname** - hostname to connect
* **-port port_number** - port number to connect (default value: 6379)
* **-ssl** - use SSL/TLS for connection
* **-ssl_ca_file path** - path to a CA certificate/bundle
* **-ssl_cert_file path** - path to a client SSL certificate
* **-ssl_key_file path** - path to a key to the client SSL certificate
* **-password password** - password for authentication
* **-username username** - username for authentication
* **-timeout timeout** - timeout value in milliseconds for connecting and sending commands (default value: -1)
* **-var variable_name** - name of the variable to be associated with the created valkey context
* **-retry_count count** - number of attempts to send a command in case of connection issues (default value: 5)
* **-reply_typed** - return a reply type along with a message

This command creates and returns a new command (handle) that can be used to send commands to a remote server.

The `-host` (and possibly `-port`) or `-path` parameter must be specified to successfully establish a connection.

Parameters `-ssl`, `-ssl_ca_file`, `-ssl_cert_file` and `-ssl_key_file` will work only if the package is built with SSL connection support (by default it is disabled). For detailed information, please see this section - [Note about SSL support](#note-about-ssl-support).

If the `-var` parameter is specified, then the command name is also assigned to a variable. If this variable is deleted, the connection will be destroyed. This is useful when using a connection inside a procedure and specifying the name of a local variable. Then, when exiting the procedure, the local variable will be deleted, releasing the connection.

If the `-var` parameter is not used, then the connection must be destroyed using the `destroy` subcommand when it is needed.

For example:

```tcl
package require valkey

# create a connection
set vkc [valkey -host "localhost" -port 7000]
# destroy the connection
$vkc destroy
```

### Available subcommands

* **handle destroy** - terminates the connection and destroys the context.
* **handle raw ?args...?** - sends the specified arguments directly to the remote server as a command and returns a response. This subcommand can be useful for sending commands that are unknown to the valkey-tcl package.
* **handle command ?args...?** - sends the specified command and its arguments to the remote server and returns a response.

### Sending commands to a remote server

Commands to the remote server can be sent using an established connection and its connection handler `handle command ?args...?`.

A list of supported commands can be found in the valkey documentation: [Valkey Commands](https://valkey.io/commands/)

For example, this can be used to set the value of the `foo` key to the string `baz` and then fetch it:

```tcl
package require valkey

# create a connection
set vkc [valkey -host "localhost" -port 7000]
# set the key "foo"
$vkc SET "foo" "baz"
# fetch a value of the key "foo"
set value [$vkc GET "foo"]
# destroy the connection
$vkc destroy
```
