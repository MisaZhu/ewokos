cmd_scripts/basic/fixdep := gcc -Wp,-MD,scripts/basic/.fixdep.d -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -Iinclude     -o scripts/basic/fixdep scripts/basic/fixdep.c  
