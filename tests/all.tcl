package require tcltest

namespace import -force ::tcltest::test

if {[info exists ::env(MEMDEBUG)]} {
    source [file join [file dirname [info script]] memleakhunter.tcl]
}

::tcltest::configure {*}$argv -singleproc true -testdir [file dirname [info script]]

set r [::tcltest::runAllTests]
if {[info exists ::env(MEMDEBUG)]} {
    incr r [check_memleaks]
}
exit $r
