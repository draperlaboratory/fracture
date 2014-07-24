#!/bin/bash
#TODO add catches for thumb and NEON
#TODO catch difference between abort and seg fault

n=""
binaries=0
target=""
triple=""
targetdir=*
debug=no
indivInst=no
decfail=0
prefix=""

while getopts paxntdhm:f: option
do
	case "${option}" 
	in
	p)	target="PPC" triple="ppc-unknown-unknown" prefix="p";;
	a)	target="ARM" n="v6" triple="arm-unknown-unknown" prexix="a";;
	x)	target="x86" triple="x86-unknown-unknown" prefix="x";;
	n)	target="ARM" triple="arm-unknown-unknown" prefix="v";;
	t)	target="ARM" n="v6" triple="arm-unknown-unknown" prefix="t";;
	d)	debug=yes;;
	h)	echo " MASS SINGLE INSTRUCTION DECOMPILER

-p	=PPC TARGET
-a	=ARM TARGET
-x	=X86 TARGET
-n	=ARM NEON TARGET (non functional)
-t	=ARM THUMB TARGET non functional)
-d	=DEBUG
-m	=Set M Atribute
-f	=Specify targed directory (If not specified the current directory will be used)
-i	=Target individual instruction file, will dec from 0x0 by default
-h	=HELP" 
		exit 0
		;;
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
mkdir -p "${target}-Instruction-Results"
cd "${target}-Instruction-Results"
echo "Test Harness for $targetdir
$triple">results.txt
date>>results.txt
hostname>>results.txt

#Look through each file in the targetdir and run it through fracture, recording the results to #ircode.tmp

for fn in ../$targetdir*; 
do
	#if the file is indeed a member of the target arch
	if file $fn | grep -q "$target"
	then
		f=$(basename $fn)		
		binaries=$((binaries+1))
		echo "dec 0x0
quit" | fracture -mattr=$n -triple=$triple $fn&>ircode.tmp
	       	printf "$target BIN,DEC 0x0,">>results.txt	

	else
		printf "$f is not a(n) $target executable">>results.txt
	fi

	if grep -q "instruction decode failed" ircode.tmp
	then	
		printf "Disas Unknown Instruction,$f\n">>results.txt

	elif grep -q "LLVM ERROR:" ircode.tmp
	then	
		#read -p "LLVM ERROR caught" -s
		grep "LLVM ERROR:" ircode.tmp | while read line; 
		do	
			#find the failure OPC
			ar=($line)
			for ((i = 0 ; i < ${#ar[@]} ; i++ )) do
				if [[ ${ar[$i]} = "=" ]]
				then 
					i=$(($i + 1))
				      	opc=${ar[$i]}
					printf "LLVM Error,$f,$opc\n">>results.txt
				fi
			done
		done	
			# If debug is enabled then print the resulting 
			# error to a file of the name of the opcode
			if [[ $debug = yes ]]
			then	
				cp ircode.tmp $f
			fi
	
	elif grep -q "Program arguments" ircode.tmp
	then
		#read -p "Most likely a seg fault" -s
		printf "SEG FAULT or ABORT,$f\n">>results.txt
		if [[ $debug = yes ]]
		then
			cp ircode.tmp $f
		
		fi
	else
		#read -p "No crash, Yay ARM support!" -s
		printf "SUCCESS,$f\n">>results.txt
	fi
done
#Print resulting statistics to file and screen
printf "Total binaries, $binaries">> results.txt
printf "\nTotal segmentation faults or aborts, ">>results.txt
grep -oc "SEG FAULT" results.txt >> results.txt
totalsuc=$(grep -oc "SUCCESS" results.txt)
printf "Total successful decompiles,$totalsuc">>results.txt
printf "\nTotal LLVM Errors, ">>results.txt
grep -oc "LLVM Error" results.txt >> results.txt
printf "Total Disassembler Related Errors,">>results.txt
grep -oc "Disas Unknown Instruction" results.txt >> results.txt
if [[ $binaries > 1 ]]
then
printf "Percent Success: ">>results.txt
echo "$totalsuc $binaries" | awk '{printf "%.2f \n", $1/$2}'>>results.txt
fi
printf "\nTotal Binaries: $binaries"
printf "\nSegmentation faults or Aborts: "
grep -oc "SEG FAULT" results.txt
totalsuc=$(grep -oc "SUCCESS" results.txt)
printf "Successful Decompiles:  $totalsuc"
printf "\nLLVM Errors: "
grep -oc "LLVM Error" results.txt
printf "Disassembler Errors: "
grep -oc "Disas Unknown Instruction" results.txt
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


































	
