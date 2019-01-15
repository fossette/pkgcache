pkgcache: pkgcache.c common.c common.h list.c list.h
	cc -v -larchive -lfetch -o pkgcache pkgcache.c common.c list.c

clean:
	rm -v pkgcache

install:
	cp -v pkgcache /usr/bin
	chmod -v a+rx /usr/bin/pkgcache

uninstall:
	rm -v /usr/bin/pkgcache
