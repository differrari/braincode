(load "~/redbuild/v3/redbuild.lisp")

; (redbuild:set-tester "tester.c")

(redbuild:quick-build (redbuild:make-instance `redbuild:redmod
        :name "brain"
        :type :bin
        :target (redbuild:native)
        :libs (list (redbuild:local-lib "uno" :lib "uno.a"))
        :srcs (list "brain.c")
) :add-dependencies t :run t :success (lambda () (print "Done") (print (redbuild:emit-compile-commands))))
