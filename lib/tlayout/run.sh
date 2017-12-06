#!/bin/bash

make || exit 1

TEST=$1
TESTDIR=~/mypass/test/
TESTFILE="$TESTDIR$1.cpp"

if [ ! -f $TESTFILE ]; then
	echo "[ERROR]Test file $1 doesn't exist!"
	exit 1
fi

echo "Step 1: Changing working directory..."
cd $TESTDIR
echo "[Step 1 done]"

# echo "Step 2: Removing the output files..."
rm llvmprof.out
rm -rf $TEST*bc
rm -rf *profile*
# rm -rf *lamp*
# rm -rf *ll
# rm -rf *.s
# rm -rf $TEST
# echo "[Step 2 done]"

echo "Step 3: Getting the simple profile data..."
clang -emit-llvm -pthread -std=c++11 -o $TEST.bc -c $TEST.cpp
opt -loop-simplify < $TEST.bc > $TEST.ls.bc
opt -insert-edge-profiling $TEST.ls.bc -o $TEST.profile.ls.bc
llc $TEST.profile.ls.bc -o $TEST.profile.ls.s
g++ -pthread -std=c++11 -o $TEST.profile $TEST.profile.ls.s /opt/llvm/Release+Asserts/lib/libprofile_rt.so
./$TEST.profile $2 $3
echo "[Step 3 done]"

echo "test pass"
# opt -load ~/mypass/Debug+Asserts/lib/tlayout.so -tlayout < $TEST.ls.bc > /dev/null
opt -load ~/mypass/Debug+Asserts/lib/tlayout.so -tlayout < $TEST.ls.bc > $TEST.tlayout.bc

# echo "Step 4: Getting Lamp profile..."
# opt -load ~/mypass/Release+Asserts/lib/slicm.so -lamp-insts -insert-lamp-profiling -insert-lamp-loop-profiling -insert-lamp-init < $TEST.ls.bc > $TEST.lamp.bc 
# llc < $TEST.lamp.bc > $TEST.lamp.s
# g++ -o $TEST.lamp.exe $TEST.lamp.s ~/mypass/tools/lamp-profiler/lamp_hooks.o
# ./$TEST.lamp.exe
# echo "[Step 4 done]"

# echo "Step 5: Running slicm pass..."
# opt -basicaa -load ~/mypass/Release+Asserts/lib/slicm.so -lamp-inst-cnt -lamp-map-loop -lamp-load-profile -profile-loader -profile-info-file=llvmprof.out -slicm < $TEST.ls.bc > $TEST.slicm.bc
# echo "[Step 5 done]"

## Compare original and modified IR
# llvm-dis $TEST.ls.bc
# llvm-dis $TEST.slicm.bc

## Once you are sure that your pass works then use mem2reg pass to convert some intermediate load stores to register transfers
# opt -basicaa -load ~/mypass/Release+Asserts/lib/slicm.so -lamp-inst-cnt -lamp-map-loop -lamp-load-profile -profile-loader -profile-info-file=llvmprof.out -slicm -mem2reg < $TEST.ls.bc > $TEST.slicm.bc

## Generate executables and ensure that your modified IR generates correct output
llc $TEST.bc -o $TEST.s
g++ -pthread -std=c++11 -o $TEST $TEST.s
llc $TEST.tlayout.bc -o $TEST.tlayout.s
g++ -pthread -std=c++11 -o $TEST.tlayout $TEST.tlayout.s

# Generate CFG
opt -dot-cfg $TEST.tlayout.bc >& /dev/null
dot -Tpdf cfg.main.dot -o $TEST.tlayout.pdf
# dot -Tpdf cfg._Z6DoWorkPv.dot -o $TESTDoWork.pdf
rm cfg.*.dot
