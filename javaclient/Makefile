dist/client.jar: src/UI.java
		javac -d . src/UI.java
		jar cvfe dist/client.jar client/UI client/*.class

clean:
		rm -f client/* dist/*

run:
		java -jar dist/client.jar