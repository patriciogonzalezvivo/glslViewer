default:
	@mkdir -p build
	@cd build && cmake .. && make
	@@cd ..

nox11:
	@mkdir build_nox11
	@cd build_nox11 && cmake -DNO_X11 .. && make
	@cd ..

wasm:
	@mkdir build_wasm
	@cd build_wasm && cmake .. -DCMAKE_TOOLCHAIN_FILE=$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake && make 
	@cd ..

clean:
	@rm -rf build*

install:
	@cd build && make install
	@cd ..

install_nox11:
	@cd build_nox11 && make install
	@cd ..