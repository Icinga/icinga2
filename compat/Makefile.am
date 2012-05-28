EXTRA_DIST=include

include:
	rm -Rf boost && mkdir -p boost && \
	bcp --boost=$(BOOST_PATH)/include tr1 smart_ptr bind function make_shared boost && \
	rm -Rf include && mkdir include && \
	mv boost/boost include/

clean:
	rm -Rf include
