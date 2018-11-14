#!/usr/bin/env bash

errored=0

for f in ./test/*.schym; do
	echo "running $(basename $f)"
	./main $f &>/dev/null
	if [[ $? -ne 0 ]]; then
		printf "\tERR\t(error code is $?)\n"
		errored=1
	else
		printf "\tOK\n"
	fi
done

if [[ $errored -ne 0 ]]; then
	exit 1
fi
