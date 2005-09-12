include Makefile.config

all: 
	make -C system
	make -C modules
	make -C modules++

	rm -f $(LIBNAME)
	$(CXX) $(LIBFLAGS) -o $(LIBNAME) system/*.o modules/*.o modules++/*.o -lm

clean:
	make -C include clean
	make -C modules clean
	make -C modules++ clean
	make -C system clean
	rm -f $(LIBNAME)
	rm -f *~

tags:
	etags --language=auto include/*.h system/*.c modules/*.c modules++/*.cpp

tagsclean:
	rm -f TAGS

install:
	test -d $(prefix)/lib/pd
	install -d $(prefix)/lib/pd/extra
	install -m 755 $(LIBNAME) $(prefix)/lib/pd/extra
	install -m 644 abs/*.pd $(prefix)/lib/pd/extra
	install -m 644 doc/*.pd $(prefix)/lib/pd/doc/5.reference
	install -d $(prefix)/lib/pd/doc/creb
	install -m 644 doc/examples/*.pd $(prefix)/lib/pd/doc/creb
