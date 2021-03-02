include settings.local

deploy: dist/server dist/serverctl
	ssh ${HOST1} "killall server; killall serverctl" || true
	scp $^ ${HOST1}:
	ssh ${HOST1} "./server"

dist/server: src/server.c src/server_lib.c src/server_clientservice.c src/server_sqlite.c
		gcc -g $^ -o $@ -lpthread -lsqlite3 -I./include

dist/serverctl: src/serverctl.c
		gcc -g $^ -o $@

java:
		cd javaclient && make

clean:
		rm -f dist/*

testnc:
		nc -N ${HOST1} 9999

testjava:
		cd javaclient && java -jar dist/client.jar &

createdb:
	ssh ${HOST1} "sqlite3 server.db" < sql/skeleton.sql
