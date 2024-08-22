
tcltest::testConstraint valkeyEnabledSsl [valkey::pkgconfig get feature-ssl]
tcltest::testConstraint valkeyDisabledSsl [expr { ! [valkey::pkgconfig get feature-ssl] }]

set ::tcltest::valkey_server_args [list]

proc valkey_fake_server { op } {

    if { $op eq "up" } {

        if { [info exists ::valkey_server_chan] } {
            return
        }

        # some random port number
        set ::tcltest::vk_port 7921
        set ::tcltest::valkey_server_chan [open "|[list \
            [::tcltest::interpreter] \
            [file join [file dirname [info script]] fake_server.tcl] \
            $::tcltest::vk_port]" w+]
        gets $::tcltest::valkey_server_chan line

    } elseif { $op eq "down" } {

        if { ![info exists ::tcltest::valkey_server_chan] } {
            return
        }

        catch { puts $::tcltest::valkey_server_chan "exit" }
        catch { close $::tcltest::valkey_server_chan }
        unset ::tcltest::valkey_server_chan

    }

}

proc valkey_server { op args } {

    if { $op eq "up" } {

        if { [info exists ::tcltest::valkey_server_chan] } {
            return
        }

        valkey_server clean

        set cmd "|[list docker run --rm \
            -e VALKEY_PORT_NUMBER=7000 -p 7000:7000 \
            -e VALKEY_TLS_ENABLED=yes \
            -e VALKEY_TLS_PORT_NUMBER=7001 -p 7001:7001 \
            -v "[file join [file dirname [info script]] certs]:/opt/bitnami/valkey/certs" \
            -e VALKEY_TLS_CERT_FILE=/opt/bitnami/valkey/certs/server-cert.pem \
            -e VALKEY_TLS_KEY_FILE=/opt/bitnami/valkey/certs/server-key.pem \
            -e VALKEY_TLS_CA_FILE=/opt/bitnami/valkey/certs/ca-cert.pem \
            {*}$args {*}$::tcltest::valkey_server_args bitnami/valkey:7.2.6] 2>@1"

        #puts "CMD: $cmd"

        set ::tcltest::valkey_server_chan [open $cmd w+]

        # Timeout is 30 seconds
        set timer_id [after 30000 [list set ::tcltest::valkey_server_ready "timeout reached"]]

        fconfigure $::tcltest::valkey_server_chan -blocking 1

        fileevent $::tcltest::valkey_server_chan readable [list apply {{ chan } {
            if { [gets $chan line] == -1 } {
                fileevent $chan readable {}
                set ::tcltest::valkey_server_ready "EOF reached"
                return
            }
            #
            # This is the line to enable output from the docker container
            #
            #puts "FILE EVENT: $line"
            #
            # This line means that server is up and running:
            #
            # 13 Aug 2024 13:12:20.450 * Ready to accept connections tcp
            #
            if { [string first {Ready to accept connections tcp} $line] != -1 } {
                fileevent $chan readable {}
                set ::tcltest::valkey_server_ready "ok"
            }
        }} $::tcltest::valkey_server_chan]

        vwait ::tcltest::valkey_server_ready

        after cancel $timer_id

        if { $::tcltest::valkey_server_ready ne "ok" } {
            catch { valkey_server down }
            puts stderr "ERROR: while starting valkey server: $::tcltest::valkey_server_ready"
            exit 1
        }

        set ::tcltest::vk_port 7000
        set ::tcltest::vk_port_ssl 7001

    } elseif { $op eq "down" } {

        if { ![info exists ::tcltest::valkey_server_chan] } {
            return
        }

        valkey_server clean

        catch { close $::tcltest::valkey_server_chan }
        unset ::tcltest::valkey_server_chan

    } elseif { $op eq "clean" } {

        if { ![catch { exec docker ps -q --filter ancestor=bitnami/valkey } ids] && $ids ne "" } {
            catch { exec docker kill {*}$ids 2>/dev/null }
        }

    }

}