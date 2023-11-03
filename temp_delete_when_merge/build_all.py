#!/usr/bin/python3

import os

os.system("rm -f *.ll")

SIG_DIR='sigs'

COMPILE_CMD = """clang-16    \
	-S    \
	-fno-discard-value-names    \
	-fno-vectorize    \
	--language=c    \
	-emit-llvm    \
	-Werror=unused-command-line-argument    \
	-Xclang    \
	-disable-O0-optnone    \
	%s    \
	-o    \
	%s    \
	-I    \
	/home/aragusa/clamav-bytecode-compiler-aragusa/headers \
	-include    \
	bytecode.h    \
	-D__CLAMBC__"""

OPTIONS_STR='--disable-loop-unrolling'
OPTIONS_STR+=" --disable-i2p-p2i-opt"
OPTIONS_STR+=" --disable-loop-unrolling"
OPTIONS_STR+=" --disable-promote-alloca-to-lds"
OPTIONS_STR+=" --disable-promote-alloca-to-vector"
OPTIONS_STR+=" --disable-simplify-libcalls"
OPTIONS_STR+=" --disable-tail-calls"
#OPTIONS_STR+=" --polly-vectorizer=none"
#OPTIONS_STR+=" --loop-vectorize"
OPTIONS_STR+=" --vectorize-slp=false"
OPTIONS_STR+=" --vectorize-loops=false"
#OPTIONS_STR+=" --disable-loop-vectorization"



internalizeAPIList = "_Z10entrypointv,entrypoint,__clambc_kind,__clambc_virusname_prefix,__clambc_virusnames,__clambc_filesize,__clambc_match_counts,__clambc_match_offsets,__clambc_pedata,__Copyright"

OPTIONS_STR+=f' -internalize-public-api-list="{internalizeAPIList}"'

PASS_STR = "function(mem2reg)"
PASS_STR+=','
PASS_STR+='clambc-remove-undefs'
PASS_STR+=',verify'
PASS_STR+=','
PASS_STR+='clambc-preserve-abis'
PASS_STR+=',verify'
PASS_STR+=',default<O3>'
#PASS_STR+=',default<O0>'
PASS_STR+=',globalopt'
PASS_STR+=',clambc-preserve-abis' #remove fake function calls because O3 has already run
PASS_STR+=',verify'
PASS_STR+=',clambc-remove-pointer-phis'
#PASS_STR+=',function(clambc-remove-pointer-phis)'
PASS_STR+=',verify'
PASS_STR+=',clambc-lowering-notfinal' # perform lowering pass
PASS_STR+=',verify'
PASS_STR+=',lowerswitch'
PASS_STR+=',verify'
PASS_STR+=',clambc-remove-icmp-sle'
PASS_STR+=',verify'
PASS_STR+=',function(clambc-verifier)'
PASS_STR+=',verify'
PASS_STR+=',clambc-remove-freeze-insts'
PASS_STR+=',verify'
PASS_STR+=',clambc-lowering-notfinal'  # perform lowering pass
PASS_STR+=',verify'
PASS_STR+=',clambc-lcompiler-helper' #compile the logical_trigger function to a
PASS_STR+=',verify'
PASS_STR+=',clambc-lcompiler' #compile the logical_trigger function to a
PASS_STR+=',verify'
PASS_STR+=',internalize'
PASS_STR+=',verify'
PASS_STR+=',clambc-rebuild'
#PASS_STR+=',verify'
#PASS_STR+=',clambc-trace'
#PASS_STR+=',verify'
#PASS_STR+=',clambc-outline-endianness-calls'
#PASS_STR+=',verify'
#PASS_STR+=',clambc-change-malloc-arg-size'
#PASS_STR+=',verify'
#PASS_STR+=',clambc-extend-phis-to-64-bit'
#PASS_STR+=',verify'
#PASS_STR+=',clambc-convert-intrinsics'
#PASS_STR+=',verify'
#PASS_STR+=',globalopt'
#PASS_STR+=',clambc-writer'
#PASS_STR+=',verify'




INSTALL_DIR=os.path.join(os.getcwd(), "..")
LOAD_STR = "--load %s/install/lib/libclambccommon.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcremoveundefs.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcpreserveabis.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcanalyzer.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambctypeanalyzer.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcremovepointerphis.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcloweringf.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcloweringnf.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcverifier.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambclogicalcompiler.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambclogicalcompilerhelper.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcremovefreezeinsts.so  " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcrebuild.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambctrace.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcoutlineendiannesscalls.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcchangemallocargsize.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcextendphisto64bit.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcregalloc.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcconvertintrinsics.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcremoveicmpsle.so " % INSTALL_DIR
LOAD_STR += " --load-pass-plugin %s/install/lib/libclambcwriter.so " % INSTALL_DIR


#wd = os.getcwd()
#os.chdir(os.path.join(INSTALL_DIR, "install"))

#os.system("tar xvf lib.tar")


#os.chdir(wd)







#OPT_CMD = 'opt-16 -S %s --passes=\"-mem2reg\" --passes=\"%s\" %s ' % (LOAD_STR, PASS_STR, OPTIONS_STR)
OPT_CMD = 'opt-16 -S %s --passes=\"%s\" %s ' % (LOAD_STR, PASS_STR, OPTIONS_STR)


#print ("Re-evaluate here")
#print ("Disabling opaque pointers here")
OPT_CMD += " -opaque-pointers=0 "
COMPILE_CMD += " -Xclang -no-opaque-pointers "

"""
#This is to find undefs.
print ("Take this part out, used to find undefs")
#PASS_STR = 'default<O3>'
OPTIONS_STR = ''
OPTIONS_STR+=" --vectorize-slp=false"
OPTIONS_STR+=" --vectorize-loops=false"
OPT_CMD = 'opt-16 -S %s --passes=\"%s\" %s ' % (LOAD_STR, PASS_STR, OPTIONS_STR)
"""



OPT_CMD += "%s -o %s"






def run(cmd):
    return os.system(cmd)


def compileFile(d, name):
    llFile = name[:-1] + "ll"

    cmd = COMPILE_CMD % (os.path.join(d,name), llFile)
    if (run(cmd)):
        return

    cmd = OPT_CMD % (llFile, llFile + ".optimized.ll")
    print (cmd)

    return run(cmd)


if '__main__' == __name__:
    for s in os.listdir(SIG_DIR):
        if (compileFile(SIG_DIR, s)):
            print (f"Failed on {s}")
            break
#        os.system("rm -f *.ll")




