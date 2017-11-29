#!/bin/bash

# ./compile.sh || exit 1

TEST=$1
TESTDIR=~/mypass/test/test_correct/
TESTFILE="$TESTDIR$1.c"

if [ ! -f $TESTFILE ]; then
	echo "[ERROR]Test file $1 doesn't exist!"
	exit 1
fi

echo "Step 1: Changing working directory..."
cd $TESTDIR
echo "[Step 1 done]"

echo "Step 2: Removing the output files..."
rm llvmprof.out
rm -rf $TEST*bc
rm -rf *profile*
rm -rf *lamp*
rm -rf *ll
rm -rf *.s
rm -rf $TEST
rm -rf $TEST.slicm
echo "[Step 2 done]"

echo "Step 3: Getting the simple profile data..."
clang -emit-llvm -o $TEST.bc -c $TEST.c
opt -loop-simplify < $TEST.bc > $TEST.ls.bc
opt -insert-edge-profiling $TEST.ls.bc -o $TEST.profile.ls.bc
llc $TEST.profile.ls.bc -o $TEST.profile.ls.s
g++ -o $TEST.profile $TEST.profile.ls.s /opt/llvm/Release+Asserts/lib/libprofile_rt.so
./$TEST.profile
echo "[Step 3 done]"

echo "Step 4: Getting Lamp profile..."
opt -load ~/mypass/Release+Asserts/lib/slicm.so -lamp-insts -insert-lamp-profiling -insert-lamp-loop-profiling -insert-lamp-init < $TEST.ls.bc > $TEST.lamp.bc 
llc < $TEST.lamp.bc > $TEST.lamp.s
g++ -o $TEST.lamp.exe $TEST.lamp.s ~/mypass/tools/lamp-profiler/lamp_hooks.o
./$TEST.lamp.exe
echo "[Step 4 done]"

echo "Step 5: Running slicm pass..."
# opt -basicaa -load ~/mypass/Release+Asserts/lib/slicm.so -lamp-inst-cnt -lamp-map-loop -lamp-load-profile -profile-loader -profile-info-file=llvmprof.out -slicm < $TEST.ls.bc > $TEST.slicm.bc
echo "[Step 5 done]"

## Compare original and modified IR
# llvm-dis $TEST.ls.bc
# llvm-dis $TEST.slicm.bc

# Once you are sure that your pass works then use mem2reg pass to convert some intermediate load stores to register transfers
opt -basicaa -load ~/mypass/Release+Asserts/lib/slicm.so -lamp-inst-cnt -lamp-map-loop -lamp-load-profile -profile-loader -profile-info-file=llvmprof.out -slicm -mem2reg < $TEST.ls.bc > $TEST.slicm.bc

## Generate executables and ensure that your modified IR generates correct output
llc $TEST.bc -o $TEST.s
g++ -o $TEST $TEST.s
llc $TEST.slicm.bc -o $TEST.slicm.s
g++ -o $TEST.slicm $TEST.slicm.s

## Generate CFG
opt -dot-cfg $TEST.slicm.bc >& /dev/null
dot -Tpdf cfg.main.dot -o $TEST.slicm.pdf
rm cfg.main.dot