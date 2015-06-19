#!/bin/bash

dir1=/tmp/json

if [ $# != 2 ]; then
	echo "Usage : $0 textfile dldfile"
	exit 1
fi

txtfile=$1
dldfile=$2

cd $dir1
rm -rf $dir1/*.json
jsonimport.bin -u root -U r1st34m -k /var/lib/mysql/mysql.sock i-ris-test $txtfile

cd $OLDPWD

if [ -f $dldfile ]; then
	mv $dldfile $dldfile.bak
fi

for jsonfile in $dir1/*.json
do
    jname=`basename $jsonfile .json`
    echo "{TYPE:EXPIRY,NAME:$jname}" >>$dldfile
    cat $jsonfile >>$dldfile
    echo >>$dldfile
done
exit 0
