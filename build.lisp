(load "~/redbuild/v3/redbuild.lisp")
(load "~/redbuild/packaging/packager.lisp")

(redbuild:set-global-target :linux)

(redbuild:make "~/redlib" "cross")

(defvar *beyond-lib* t)
(defvar *beyond-interpreter* t)
(redbuild:build-dep "~/beyond")

(redbuild:build-dep "~/uno")

(redbuild:quick-build (redbuild:make-instance `redbuild:redmod
        :name "brain"
        :type :bin
        :target (redbuild:dyn-target)
        :libs (list (redbuild:local-lib "uno" :lib "uno.a") (redbuild:local-lib "beyond" :lib "imaginal.a"))
        :srcs (list "brain.c" "tree_layout.c" "file/file_source.c" "shell/shell_source.c")
) :add-dependencies t :run t :debug-symbols t :success (lambda () 
    (print (redbuild:emit-compile-commands))
    (generate-build (make-instance `pack
        :name "brain"
        :version "0.1a1"
        :author "di"
        :exec "brain"
        :categories "Developer"
    ) (redbuild:native))
    ; (redbuild:install "brain.red")
    ; (redbuild:make "~/os" "run")
    (print "Done")
))
