
for x in *.en.html ; do
    y=`echo -n $x | sed 's/[.]en[.]html/.md/'`
    echo $x $y
    html2text.py $x > $y
    sed -i 's/[.]en[.]html//g' $y
    sed -i 's+pics/+https://raw.githubusercontent.com/vnaydionov/yb-orm/master/doc/pics/+g' $y
done

for x in *.ru.html ; do
    y=`echo -n $x | sed 's/\(.*\)[.]ru[.]html/ru_\1.md/'`
    echo $x $y
    html2text.py $x > $y
    sed -i 's/\(.*\)[.]ru[.]html/ru_\1.md/g' $y
    sed -i 's+pics/+https://raw.githubusercontent.com/vnaydionov/yb-orm/master/doc/pics/+g' $y
done

