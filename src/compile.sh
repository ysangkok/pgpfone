#!/bin/bash
lines=$(grep -hrE "^#include.*\.c" . | uniq | cut -f2 -d'"')
echo $lines

function isexcluded() {
        echo "$lines" | while read line
        do
                if [ "$line" = "$1" ]; then
                        return 1
                fi
        done

        if [ $? -eq 0 ]; then
                return 0
        else
                return 1
        fi
}

for i in $(find -L . -iname '*.cpp' -or -iname '*.c' -type f )
do
	export WINEDEBUG="-all"
	res="$(echo $(basename $i) | cut -d'.' -f1).obj"
	if [ -f "$res" ]; then continue; fi
	isexcluded "$(basename $i)"
	if [ $? -ne 0 ]; then continue; fi
	wine cmd /c compile.bat $i
done
