#include "redbuild.h"
void main(){
	rebuild_self();
	if (!make_run("~/uno", "")){
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
}
