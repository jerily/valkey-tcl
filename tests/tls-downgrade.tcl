package require valkey 1.0.1
lassign $argv port ca
set client [valkey -host localhost -port $port -ssl -ssl_ca_file $ca \
    -ssl_cert_file [file join [file dirname $ca] server.pem] \
    -ssl_key_file [file join [file dirname $ca] server.key] \
    -username appuser -password tls-test-password -timeout 1000 -retry_count 1]
puts READY
flush stdout
gets stdin
if {![catch {$client PING}]} {error "plaintext retry unexpectedly succeeded"}
$client destroy
puts REJECTED
