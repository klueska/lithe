#!/bin/sh
# Convert git log to GNU-style ChangeLog file.
COPYRIGHT="\
Copyright (c) 2011 The Regents of the University of California\nCopying and distribution of this file, with or without modification, are\npermitted provided the copyright notice and this notice are preserved.\n
"
git log --date=short --pretty=format:"%ad %aN <%ae> %n%n%x09* %s%d%n%b" > ChangeLog
echo "" >> ChangeLog
echo $COPYRIGHT >> ChangeLog

