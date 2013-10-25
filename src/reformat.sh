#!/bin/bash
# for more info refere to http://astyle.sourceforge.net/astyle.html
FILELIST=`ls *.c *.h *.cpp`
for SRCFILE in $FILELIST
do
    astyle -A3 -T4 -f -p  -k3 -y  $SRCFILE
done
rm *.style~

