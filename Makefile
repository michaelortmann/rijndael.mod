# Makefile for src/mod/${MYNAME}.mod/

MYNAME=rijndael
srcdir = .


doofus:
	@echo ""
	@echo "Let's try this from the right directory..."
	@echo ""
	@cd ../../../ && make

static: ../${MYNAME}.o

modules: ../../../${MYNAME}.$(MOD_EXT)

./rijndael_cipher.o: rijndael_cipher.c
	$(CC) $(CFLAGS) $(CPPFLAGS) rijndael_cipher.c -c -o rijndael_cipher.o

../${MYNAME}.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -DMAKING_MODS -c $(srcdir)/${MYNAME}.c
	@rm -f ../${MYNAME}.o
	mv ${MYNAME}.o ../

../../../${MYNAME}.$(MOD_EXT): ../${MYNAME}.o ./rijndael_cipher.o
	$(LD) -o ../../../${MYNAME}.$(MOD_EXT) ../${MYNAME}.o ./rijndael_cipher.o $(XLIBS)
	$(STRIP) ../../../${MYNAME}.$(MOD_EXT)

depend:
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $(srcdir)/${MYNAME}.c > .depend

clean:
	@rm -f .depend *.o *.$(MOD_EXT) *~
distclean: clean

#safety hash
rijndael.o: .././rijndael.mod/rijndael.c ../../../src/mod/module.h \
 ../../../src/main.h ../../../config.h ../../../eggint.h ../../../lush.h \
 ../../../src/lang.h ../../../src/eggdrop.h ../../../src/compat/in6.h \
 ../../../src/flags.h ../../../src/cmdt.h ../../../src/tclegg.h \
 ../../../src/tclhash.h ../../../src/chan.h ../../../src/users.h \
 ../../../src/compat/compat.h ../../../src/compat/snprintf.h \
 ../../../src/compat/strlcpy.h ../../../src/mod/modvals.h \
 ../../../src/tandem.h .././rijndael.mod/rijndael.h \
 .././rijndael.mod/aes.h
