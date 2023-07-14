#add_llvm_loadable_module( MyPass
#    MyPass.cpp
#
#    PLUGIN_TOOL
#    opt
#)


add_llvm_pass_plugin( MyPass
    MyPass.cpp

)




