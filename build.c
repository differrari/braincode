#include "redbuild.h"
int main(){
	int __return_val;
	rebuild_self();
	if (!make_run("~/uno", "")){
		__return_val = 1;
		goto defer;
	}
	
	if (!make_run("~/redlib", "")){
		__return_val = 1;
		goto defer;
	}
	
	new_module("braincode");
	set_name("braincode");
	set_package_type(package_red);
	set_target(target_redacted);
	ignore_source("build.c");
	source_all("c");
	add_local_dependency("~/uno", "~/uno/uno.a", "~/uno", true);
	if (compile()){
		gen_compile_commands(0);
		install("../../../applications");
		make_run("~/os", "run");
	}
	
	defer:
	emit_compile_commands();
	return __return_val;
}
