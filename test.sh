#!/usr/bin/env bash

for f in ./test/*.schym; do
	./main $f &>/dev/null
	if [[ $? -ne 0 ]]; then
		echo "running $f failed with error code $?"
		exit $?
	fi
done
