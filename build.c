#include "redbuild.h"
int main(int argc, strarr argv){
	int __return_val;
	parse_arguments(argc, argv);
	rebuild_self(false);
	if (global_target == target_none){
		set_global_target(target_redacted);
	}
	
	if (!redbuild_run("~/uno")){
		__return_val = 1;
		goto defer;
	}
	
	new_module("braincode");
	set_name("braincode");
	set_package_type(package_red);
	ignore_source("build.c");
	source_all("c");
	add_local_dependency("~/uno", "~/uno/uno.a", "~/uno", true);
	if (compile()){
		gen_compile_commands(0);
		if (global_target == native_target){
			run();
		} else {
			install("../../../applications");
			make_run("~/os", "run");
		}
	}
	
	defer:
	emit_compile_commands();
	return __return_val;
}
