while true
do
	FILES_TO_WATCH="Makefile $(find ./src/ -iname "*.[ch]")"
	make
	if [ $? -eq 0 ]
	then
		sudo mspdebug rf2500 "prog build/main.elf"
	fi
	inotifywait -e modify -e move_self $FILES_TO_WATCH
	sleep 1
done
