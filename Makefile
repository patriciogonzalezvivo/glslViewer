default:
	@mkdir build
	@cd build && cmake .. && make && cd ..

nox11:
	@mkdir build
	@cd build && cmake -DNO_X11 .. && make && cd ..

clean:
	@rm -rf build

install:
	@cd build
	@sudo make install
	@cd ..