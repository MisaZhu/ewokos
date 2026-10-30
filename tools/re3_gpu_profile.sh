#!/bin/sh
# Compare the two GPU vertex-stream paths in a stationary render benchmark.
# Usage: tools/re3_gpu_profile.sh [root@host] [sample-seconds] [stream|texture]
set -eu
RE3_PROFILE_HOST=${1:-root@192.168.8.18}
RE3_PROFILE_DURATION=${2:-30}
RE3_PROFILE_PASS=${RE3_SSH_PASSWORD:-root}
RE3_PROFILE_KIND=${3:-stream}
export RE3_PROFILE_HOST RE3_PROFILE_DURATION RE3_PROFILE_PASS RE3_PROFILE_KIND
case "$RE3_PROFILE_KIND" in
    stream) RE3_PROFILE_MODES="0 1" ;;
    texture) RE3_PROFILE_MODES="1" ;;
    *) echo "comparison must be stream or texture" >&2; exit 2 ;;
esac
case "$RE3_PROFILE_DURATION" in
    ''|*[!0-9]*) echo "sample duration must be an integer" >&2; exit 2 ;;
esac
[ "$RE3_PROFILE_DURATION" -ge 10 ] || { echo "sample duration must be at least 10 seconds" >&2; exit 2; }
command -v expect >/dev/null 2>&1 || { echo "expect is required" >&2; exit 2; }
for RE3_PROFILE_DIRECT in $RE3_PROFILE_MODES; do
    RE3_PROFILE_LOG=${TMPDIR:-/tmp}/re3-profile-${RE3_PROFILE_KIND}-${RE3_PROFILE_DIRECT}.log
    export RE3_PROFILE_DIRECT RE3_PROFILE_LOG
    expect <<'EXPECT_SCRIPT' >/dev/null
set timeout 30
log_file -noappend $env(RE3_PROFILE_LOG)
proc remote {args} {
    global spawn_id env
    set saved $spawn_id
    spawn ssh -T -o ConnectTimeout=8 -o StrictHostKeyChecking=no $env(RE3_PROFILE_HOST) {*}$args
    expect -re {assword:}
    send -- "$env(RE3_PROFILE_PASS)\r"
    expect eof
    set result $expect_out(buffer)
    catch wait
    set spawn_id $saved
    return $result
}
spawn ssh -tt -o ConnectTimeout=8 -o StrictHostKeyChecking=no $env(RE3_PROFILE_HOST)
expect -re {assword:}
send -- "$env(RE3_PROFILE_PASS)\r"
expect -re {#}
set texture_lru [expr {$env(RE3_PROFILE_KIND) eq "texture" ? 2 : 0}]
foreach cmd [list "cd /apps/re3" "export RE3_RENDER_BENCHMARK=1" "export PGL_PROFILE=1" "export PGL_RW_GPU=1" "export PGL_RW_DIRECT=$env(RE3_PROFILE_DIRECT)" "export PGL_RW_TEXTURE_LRU=$texture_lru"] {
    send -- "$cmd\r"
    expect -re {#}
}
send -- "./re3\r"
set started [clock seconds]
set frozen 0
set sampled 0
while {[clock seconds]-$started < 180+$env(RE3_PROFILE_DURATION)} {
    if {$frozen && !$sampled && [clock seconds]-$frozen >= 10} {
        puts "BENCH_SAMPLE_BEGIN"
        set sampled [clock seconds]
    }
    if {$sampled && [clock seconds]-$sampled >= $env(RE3_PROFILE_DURATION)} break
    set timeout 1
    expect {
        -re {.+} {
            if {!$frozen && [string first {RE3_RENDER_BENCHMARK frozen street scene} $expect_out(0,string)] >= 0} {
                set frozen [clock seconds]
            }
        }
        timeout {}
        eof {exit 1}
    }
}
puts "BENCH_SAMPLE_END"
set timeout 30
# Create the live-detach marker with a command available on EwokOS. Do not
# rely on a Unix touch command, or kill a process during GPU submission.
remote cp /apps/re3/re3 /apps/rwdetach
set timeout 15
expect {
    -re {PGL RW GPU detached by request} {}
    timeout {error "GPU detach was not acknowledged; refusing to kill"}
}
set result [remote ps]
if {![regexp {root +([0-9]+)[^\r\n]*/apps/re3/re3(?: |\r|\n)} $result unused pid]} {
    puts "Unable to find benchmark process for cleanup"
    exit 2
}
remote kill $pid
expect -re {#}
send -- "rm /apps/rwdetach\r"
expect -re {#}
send -- "exit\r"
expect eof
if {!$sampled} {exit 3}
EXPECT_SCRIPT
    echo "GPU direct=$RE3_PROFILE_DIRECT: $RE3_PROFILE_LOG"
    rg 'PGL_PROFILE submits=|PGL_PROFILE rw_texture|BENCH_SAMPLE|ordered raster ready' "$RE3_PROFILE_LOG" | tail -30
done
