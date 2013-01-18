gadmd: gadmd.c daemonizer.c network.c logger.c player.c gameroom.c gamemaster.c deck.c computer.c
	gcc gadmd.c -o gadmd -lpthread
	gcc player.c -o player
	gcc computer.c -o computer

clean:
	rm gadmd player computer
	kill `cat /tmp/lucky9.lock`
	rm /tmp/lucky9.log
