#!/bin/bash

#program that will grab all ARM binaries in a directory and check if they run, if not it will print the opc or error to results.txt
#commented out read commands can be read like comments
#implement stack dump

n=""
binaries=0
target=""
stackdump=no
targetdir=*
checksum=no
indivInst=no

while getopts ipaxsdhcm:f: option
do
	case "${option}" 
	in
	i)	indivInst=yes;;
	p)	target="PPC";;
	a)	target="ARM" n="v6";;
	x)	target="x86";;
	s)	stackdump=yes;;
	d)	stackdump=yes checksum=yes;;
	h)	echo " FRACTURE DIRECTORY MASS DECOMPILER

-p	=PPC TARGET
-a	=ARM TARGET
-x	=X86 TARGET
-s	=Enable stackdump print on failure (quite large)
-d	=DEBUG (ALL ENABLED)
-m	=Set M Atribute
-f	=Specify targed directory (If not specified the current directory will be used)
-c	=Enable checksum print on failure
-i	=Target individual instruction filew, will dec from 0x0 by default
-h	=HELP" 
		exit 0
		;;
	c)	checksum=yes;;
	m)	n="$OPTARG";;
	f)	targetdir="$OPTARG";;	
	[?])	echo "Unknown flag option";;
	esac
done	
if [[ $target = "" ]]
then
	echo " You must specify a target, type -h for help"
	exit 1;
fi

echo "Test Harness for $targetdir
Tripple = $target - unknown - unknown">results.txt
date>>results.txt
hostname>>results.txt
echo -e "\n  TYPE   |  DEC LOC   | RESULT\n">>results.txt

for fn in $targetdir*;
#for fn in *; 
do
	if file $fn | grep -q "$target"
	then
		binaries=$((binaries+1))
		#echo "Filename is >>> $fn"
		echo "sym .text		
quit" | fracture -mattr=$n $fn&>location.tmp
		#read -p "SYM written to a temp... " -s
	if [[ $indivInst = yes ]]
	then
		#read -p "Hey you pressed -i" -n1 -s
		echo "dec 0x0
quit" | fracture -mattr=$n $fn&>ircode.tmp
		printf "$target BIN | DEC 0x0  | ">>results.txt	
	elif grep -q "main" location.tmp
	then	
		#read -p "Got main, running dec..." -s		
		echo "dec main
quit" | fracture -mattr=$n $fn&>ircode.tmp
		printf "$target  ELF |  DEC MAIN  | ">>results.txt
		#read -p "Saved dec main to a temp..."
	else	
		#echo "binary is stripped"
		#grab the last word 0x0000aaaa	
		location=$(tail -c11 location.tmp)
		#read -p "Got the location >> $location " -n1 -s
		printf "$target STRIPPED | $location | ">>results.txt
		echo "dec $location
quit" | fracture -mattr=$n $fn&>ircode.tmp
		#read -p "Saved DEC to a temp... " -n1 -s				
	fi
	
	if grep -q "LLVM ERROR:" ircode.tmp
	then	
		#read -p "We crashed, llvm error" -s
		grep "LLVM ERROR:" ircode.tmp | while read line; 
		do	
			ar=($line)
			for ((i = 0 ; i < ${#ar[@]} ; i++ )) do
				if [[ ${ar[$i]} = "=" ]]
				then 
					i=$(($i + 1))
				      	opc=${ar[$i]}
					printf "FAIL OPC: $opc        | $fn\n">>results.txt
				
				fi
			done
		if [[ $checksum = yes ]]
		then	
			file $fn>>results.txt
			
		fi
		
		done
		
	elif grep -q "Program arguments" ircode.tmp
	then
		printf "SEG FAULT or ABORT   | $fn\n">>results.txt
		if [[ $checksum = yes ]]
		then
			grep -A100 "PrintStackTrace" ircode.tmp>>results.txt
			printf "\n">>results.txt
		fi
	else
		#read -p "No crash, Yay ARM support!" -s	
		printf "SUCCESS\n">>results.txt
	fi
	
	else
		echo -e "\n$fn is not a(n) $target executable"
	fi
done

#Print results to file basic ones to screen
printf "\nTotal binaries $binaries" >> results.txt
printf "\nTotal segmentation faults or aborts: ">>results.txt
grep -oc "SEG FAULT" results.txt >> results.txt
totalsuc=$(grep -oc "SUCCESS" results.txt)
printf "Total successful decompiles:  $totalsuc">>results.txt
printf "\nTotal LLVM crashes: ">>results.txt
grep -oc "FAIL OPC" results.txt >> results.txt
if [[ $binaries > 1 ]]
then
printf "Percent Success: ">>results.txt
echo "$totalsuc $binaries" | awk '{printf "%.2f \n", $1/$2}'>>results.txt
fi
printf "\nTotal binaries $binaries"
printf "\nTotal segmentation faults or aborts: "
grep -oc "SEG FAULT" results.txt
totalsuc=$(grep -oc "SUCCESS" results.txt)
printf "Total successful decompiles:  $totalsuc"
printf "\nTotal LLVM crashes: "
grep -oc "FAIL OPC" results.txt
if [[ $binaries > 1 ]]
then
printf "Percent Success: "
echo "$totalsuc $binaries" | awk '{printf "%.2f \n", $1/$2}'
fi
#clean up and delete temp files
function finish {
find * -type f \( -name 'location.tmp' -o -name 'ircode.tmp' -o -name 'fracturemdec.sh~' \) -delete
}
trap finish EXIT


