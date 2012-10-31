cwm
===

The calm window manager (cwm)

Tracking CVS
============

**Only use the cvs-tracking branch to sync with upstream.**

This is the recommended procedure for tracking changes to the
upstream CVS repository; remember to only run the git-cvsimport
command below while the cvs-tracking branch is checked out.

First, copy the cvs-authors file to .git/cvs-authors.  Next,
checkout the cvs-tracking branch.  Finally, run:
    $ git cvsimport -d anoncvs@anoncvs.openbsd.org:/cvs -a -r upstream xenocara/app/cwm
