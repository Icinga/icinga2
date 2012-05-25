EXTRA_DIST=include

dist-hook:
	mkdir -p boost && \
	bcp --boost=$(BOOST_PATH)/include tr1 smart_ptr bind function make_shared boost && \
	rm -Rf include && \
	mkdir include && \
	mv boost/boost include/ && \
	rm -Rf boost
