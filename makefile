#main makefile

-include make_config

#rule for generating binary
$(binary): folders $(dep_files) $(header_files) $(bin_dir)libHorde3D.so $(lib_dir)libsquirrel.a $(lib_dir)libSDL2.a content
	@rm -f $(logfile)
	@#cd content && $(MAKE) target=../$(content_dir) --no-print-directory
	$(MAKE) -f build.makefile -j 2 --no-print-directory 2>$(logfile); cat $(logfile);

#rule for generating depend files
$(dep_dir)%.d: $(src_dir)%.cpp $(dep_dir)
	@$(CC) -MM $(c_options) -o .dep $<
	@sed 's:$(subst .d,.o,$(@F)):obj/debug/$*\.o obj/release/$*\.o:' <.dep >$@
	@rm -f .dep

#empty rule for headers
$(src_dir)%.h:
	

folders:
	@mkdir -p include dep/tex_op obj/debug/tex_op obj/release/tex_op bin/debug bin/release lib SQUIRREL3/lib

#rule for generating assembly file
$(asm_dir)%.s: $(src_dir)%.cpp
	$(CC) -c -S $(c_options) -o $@ $<

$(bin_dir)libHorde3D.so:
	cd $(h3dsdk) && cmake -G "Unix Makefiles" && $(MAKE)
	cp -f $(h3dso) $(bin_dir)libHorde3D.so

$(lib_dir)libsquirrel.a:
	mkdir -p include/squirrel
	cd $(squirrel) && $(MAKE) sq64
	cp -f $(sqlib) $(lib_dir)libsquirrel.a
	cp -f $(sqstdlib) $(lib_dir)libsqstdlib.a

$(lib_dir)libSDL2.a:
	@mkdir -p SDL2/build include/SDL2
	cd SDL2/build && cmake -G "Unix Makefiles" ../$(SDL_dir) && make SDL2-static
	cp -f SDL2/build/libSDL2.a $(lib_dir)

#rule for generating assembly code
asm: $(asm_files)

#rule for generating dependency files
dep: $(dep_files)
	
.PHONY: clean content asm dep clean_dep
clean:
	rm -f $(binary)
	find $(obj_dir) -type f -exec rm {} \;
	find $(dep_dir) -type f -exec rm {} \;

clean_dep:
	rm -f $(dep_files)

content:
	cd content && $(MAKE) target=../$(content_dir) --no-print-directory
