#!/bin/bash
#TODO add catches for thumb and NEON
#TODO catch difference between abort and seg fault


mkdir -p "${target}-Instruction-Results"
cd "${target}-Instruction-Results"
echo "Test Harness for $targetdir
$triple">results.txt
date>>results.txt
hostname>>results.txt

#Look through each file in the targetdir and run it through fracture, recording > ircode.tmp

for fn in ../$targetdir*
do
		#read -p "Inside for loop with $fn" -s
		f=$(basename $fn
		binaries=$((binaries+1))
		echo "dec 0x0
quit" | fracture -mattr=$n -triple=$triple $fn&>ircode.tmp
	       	printf "$target BIN,DEC 0x0,">>results.txt

	if grep -q "instruction decode failed" ircode.tmp
	then
		printf "Disas Unknown Instruction, $f\n">>results.txt

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
					printf "llvm Error, $f,$opc\n">>results.txt
				fi
			done
		done
			# If debug is enabled then print the resulting
			# error to a file of the name of the opcode
			if [[ $debug = yes ]]
			then
				cp ircode.tmp $f
				if [[ $infile = yes ]]
				then
					dbgoutput=$(<ircode.tmp>)
					printf "\n,$dbgoutput\n">>results.txt
				fi
			fi
	elif grep -q "Program arguments" ircode.tmp
	then
		#read -p "Most likely a seg fault" -s
		printf "SEG FAULT or ABORT, $f\n">>results.txt
		if [[ $debug = yes ]]
		then
			cp ircode.tmp $f
			if [[ $infile = yes ]]
			then
				dbgoutput=$(<ircode.tmp)
				printf "\n,$dbgoutput\n">>results.txt
			fi
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
printf "\nTotal LLVM  Errors, ">>results.txt
grep -oc "llvm Error" results.txt >> results.txt
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
printf "\nLLVM  Errors: "
grep -oc "llvm Error" results.txt
printf "Disassembler Errors: "
grep -oc "Disas Unknown Instruction" results.txt
if [[ $binaries > 1 ]]
then
printf "Percent Success: "
echo "$totalsuc $binaries" | awk '{printf "%.2f \n", $1/$2}'
if [[ $target = arm ]]
then
	for i in $(<results.txt)
	do
		varcheck=${i:0:1}
		if [[ $varcheck = V ]]
		then
			#read -p "Neon Instruction" -s
			allV=$((allV+1))
		elif [[ $varcheck = t ]]
		then
			#read -p "Thumb or Thumb2 instruction" -s
			allV=$((allV+1))
		fi
	done
fi
printf "Success of ARM v6 instructions: ~"
totalreal=$((binaries-allV))
echo "$totalsuc $totalreal" | awk '{printf "%.2f \n", $1/$2}'
fi
#clean up and delete temp files
function finish {
#find * -type f \( -name 'location.tmp' -o -name 'ircode.tmp' -o -name 'fracturemdec.sh~' \) -delete
}
trap finish EXIT

