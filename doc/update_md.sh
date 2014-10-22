#!/bin/sh

for x in *.en.html *.ru.html ; do
    y=`echo -n $x | sed 's/\(.*\)[.]\(en\|ru\)[.]html/\2_\1.md/'`
    echo $x $y
    python ./html2text.py $x > $y
    sed -i 's/\([A-Za-z0-9]*\)[.]\(en\|ru\)[.]html/\2_\1/g' $y
    sed -i 's+pics/+https://raw.githubusercontent.com/vnaydionov/yb-orm/master/doc/pics/+g' $y
done
