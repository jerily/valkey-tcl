package require valkey 1.0.1
lassign $argv port directory

proc assert {expression} {
    if {![uplevel 1 [list expr $expression]]} {error "assertion failed: $expression"}
}

set options [list -host localhost -port $port -ssl -timeout 1000 -retry_count 1 \
    -ssl_ca_file [file join $directory ca.pem] -username appuser -password tls-test-password]
set client [valkey {*}$options]
assert {[$client PING] eq "PONG"}
assert {[$client SET tls-test value] eq "OK"}
assert {[$client GET tls-test] eq "value"}

# Reconnect must restore both TLS and ACL authentication.
set admin [valkey {*}$options]
set old_id [$client raw CLIENT ID]
$admin raw CLIENT KILL ID $old_id
assert {[$client GET tls-test] eq "value"}
assert {[$client raw CLIENT ID] != $old_id}
$client destroy

set client [valkey {*}$options -host 127.0.0.1]
assert {[$client PING] eq "PONG"}
$client destroy

assert {[catch {valkey {*}$options -host 127.0.0.2} message]}
assert {[string match {*SSL channel*} $message]}
assert {[catch {valkey {*}$options -ssl_ca_file [file join $directory other-ca.pem]} message]}
assert {[string match {*SSL channel*} $message]}
assert {[catch {valkey {*}$options -password wrong} message]}
assert {[string match {*authenticating*} $message]}
assert {[catch {valkey -host localhost -port $port -ssl_ca_file [file join $directory ca.pem]}]}

# OpenSSL's default trust paths must work without an explicit -ssl_ca_file.
set ::env(SSL_CERT_FILE) [file join $directory ca.pem]
set client [valkey -host localhost -port $port -ssl -timeout 1000 \
    -username appuser -password tls-test-password]
assert {[$client PING] eq "PONG"}
$client destroy

# Require a client certificate and check the existing mTLS options.
$admin raw CONFIG SET tls-auth-clients yes
set client [valkey {*}$options -ssl_cert_file [file join $directory server.pem] \
    -ssl_key_file [file join $directory server.key]]
assert {[$client PING] eq "PONG"}
$client destroy
$admin destroy
puts "TLS, ACL, reconnect, identity, CA and client-certificate checks passed"
