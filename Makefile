CFLAGS= -msse -I/usr/include/X11 -W -msse2 -std=c99 -fomit-frame-pointer 

headtrack: headtrack.c
	gcc $(CFLAGS) -m64 -mtune=amdfam10 -Wno-unused-parameter -Wno-unused-value -Wno-unused-function -W -Wall -O3 -I/usr/include/X11 `pkg-config --cflags --libs opencv x11 gtk+-2.0 gthread-2.0` -o headtrack headtrack.c

headtrack32: headtrack.c
	gcc -std=c99 -Wno-unused-parameter -Wno-unused-value -Wno-unused-function -W -Wall -O2 -I/usr/include/X11 `pkg-config --cflags --libs opencv x11` -m32 -o headtrack32 headtrack.c

deb: headtrack
	cp headtrack ./debian/usr/bin/headtrack
	dpkg --build debian
	mv debian.deb headtrack.deb

installdeb: deb
	sudo dpkg -i headtrack.deb

install: headtrack
	cp headtrack /usr/bin/headtrack

uninstall: 
	rm -f /usr/bin/headtrack

clean: 
	rm -f ./headtrack
	rm -f ./headtrack.deb
	rm -f ./headtrack32
	rm -f ./debian/usr/bin/headtrack

run: 
	./headtrack
