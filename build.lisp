(load "~/redbuild/v3/redbuild.lisp")

; (redbuild:set-tester "tester.c")

(redbuild:make "~/redlib" "cross")
(redbuild:build-dep "~/beyond")
(redbuild:build-dep "~/uno")

(redbuild:quick-build (redbuild:make-instance `redbuild:redmod
        :name "brain"
        :type :bin
        :target (redbuild:native)
        :libs (list (redbuild:local-lib "uno" :lib "uno.a") (redbuild:local-lib "beyond" :lib "imaginal.a"))
        :srcs (list "brain.c" "tree_layout.c" "file/file_source.c" "shell/shell_source.c")
) :add-dependencies t :run t :debug-symbols t :success (lambda () (print "Done") (print (redbuild:emit-compile-commands))))
