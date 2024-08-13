
set ::valkey_server_args [list]

proc valkey_fake_server { op } {

    if { $op eq "up" } {

        if { [info exists ::valkey_server_chan] } {
            return
        }

        # some random port number
        set ::vk_port 7921
        set ::valkey_server_chan [socket -server [list apply {{args} {}}] $::vk_port]

    } elseif { $op eq "down" } {

        if { ![info exists ::valkey_server_chan] } {
            return
        }

        catch { close $::valkey_server_chan }
        unset ::valkey_server_chan

    }

}

proc valkey_server { op args } {

    if { $op eq "up" } {

        if { [info exists ::valkey_server_chan] } {
            return
        }

        valkey_server clean

        set cmd "|[list docker run --rm -e VALKEY_PORT_NUMBER=7000 -p 7000:7000 \
            {*}$args {*}$::valkey_server_args bitnami/valkey:7.2.6] 2>@1"

        #puts "CMD: $cmd"

        set ::valkey_server_chan [open $cmd w+]

        # Timeout is 30 seconds
        after 30000 [list set ::valkey_server_ready "timeout reached"]

        fconfigure $::valkey_server_chan -blocking 1

        fileevent $::valkey_server_chan readable [list apply {{ chan } {
            if { [gets $chan line] == -1 } {
                fileevent $chan readable {}
                set ::valkey_server_ready "EOF reached"
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
                set ::valkey_server_ready "ok"
            }
        }} $::valkey_server_chan]

        vwait ::valkey_server_ready

        if { $::valkey_server_ready ne "ok" } {
            catch { valkey_server down }
            puts stderr "ERROR: while starting valkey server: $::valkey_server_ready"
            exit 1
        }

        set ::vk_port 7000

    } elseif { $op eq "down" } {

        if { ![info exists ::valkey_server_chan] } {
            return
        }

        valkey_server clean

        catch { close $::valkey_server_chan }
        unset ::valkey_server_chan

    } elseif { $op eq "clean" } {

        if { ![catch { exec docker ps -q --filter ancestor=bitnami/valkey } ids] && $ids ne "" } {
            catch { exec docker kill {*}$ids 2>/dev/null }
        }

    }

}