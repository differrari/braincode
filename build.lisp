(load "~/redbuild/v3/redbuild.lisp")
(load "~/redbuild/packaging/packager.lisp")

(defvar *terminal-mode* t)
(redbuild:set-global-target :redacted)

(redbuild:make "~/redlib" "cross")

(defvar *beyond-lib* t)
(defvar *beyond-interpreter* t)
(redbuild:build-dep "~/beyond")

(defmacro pname () (if *terminal-mode* "term" "brain"))

(redbuild:build-dep "~/uno")

(redbuild:quick-build (redbuild:make-instance `redbuild:redmod
        :name (pname)
        :type :bin
        :target (redbuild:dyn-target)
        :libs (list (redbuild:local-lib "uno" :lib "uno.a") (redbuild:local-lib "beyond" :lib "imaginal.a"))
        :srcs (redbuild:all-sources-ignoring "c" (list "main.c" "build.c"))
        :flags (list "-g" (if *terminal-mode* "-DTERMINAL"))
) :add-dependencies t :run (not (eq (redbuild:dyn-target) :redacted)) :debug-symbols t :success (lambda () 
    (redbuild:emit-compile-commands)
    (generate-build (make-instance `pack
        :name (pname)
        :version "0.1a1"
        :author "di"
        :exec (pname)
        :categories "Developer"
        :id (concatenate `string "com.diferrari." (pname))
    ) (redbuild:dyn-target))
    (if (eq (redbuild:dyn-target) :redacted)
        (progn
            (redbuild:install (concatenate `string (pname) ".red"))
            (redbuild:make "~/os" "run")
        )
    )
    (print "Done")
))
