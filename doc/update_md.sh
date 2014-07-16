
for x in *.en.html ; do
    y=`echo -n $x | sed 's/[.]en[.]html//'`
    echo $x $y
    html2text.py $x > $y
    sed -i 's/[.]\(en\|ru\)[.]html//g' $y
done

