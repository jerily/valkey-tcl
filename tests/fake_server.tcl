socket -server [list apply {{ sock addr port } {
    fconfigure $sock -buffering line -translation crlf
    fileevent $sock readable [list apply {{ sock } {
        gets $sock line
        if { $line eq "PING" } {
            puts $sock "+PONG"
        }
    }} $sock]
}}] [lindex $argv 0]

puts "ready"

fileevent stdin readable [list set ::done 1]
vwait ::done
