default:
	@mkdir build
	@cd build && cmake .. && make && cd ..

nox11:
	@mkdir build
	@cd build && cmake -DNO_X11 .. && make && cd ..

wasm:
	@mkdir build
	@cd build && cmake .. -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake && make && cd ..

clean:
	@rm -rf build

install:
	@cd build
	@sudo make install
	@cd ..