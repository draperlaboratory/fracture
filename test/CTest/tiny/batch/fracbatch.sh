#!/bin/bash

FRAC=../frac-arm
ODIR=log
IDIR=armbins

for i in `ls $IDIR/`
do
  mkdir -p ./$ODIR
  echo "dec func_1" | $FRAC $IDIR/$i 2> $ODIR/$i.err
done
