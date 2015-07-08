#!/bin/bash

FRAC=../frac-arm
ODIR=log
IDIR=armbins

for i in `ls $IDIR/`
do
  mkdir -p ./$ODIR
  echo "dis func_1" | $FRAC $IDIR/$i &> /dev/null
  if [ $? -eq 0 ]
  then
    echo -n $i
  fi
done
