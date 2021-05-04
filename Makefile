CC = gcc
NASM = nasm
USE_MMX = 0
USE_CSS = 1
A52DIR := $(shell expr a52dec* )

DST=/usr/bin

ARCH = $(shell uname -m)
OBJDIR := ${ARCH}

ifeq (${ARCH}, alpha)
  USE_MMX = 0
  CFLAGS ?= -O4 -arch ev67 -ieee -accept c99_keywords -gcc_messages
endif

ifeq (${ARCH}, i686)
  USE_MMX = 1
  CFLAGS ?= -O2 -fomit-frame-pointer -falign-loops=2 -falign-jumps=2 -falign-functions=2
  CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
endif

ifeq (${ARCH}, x86_64)
  CFLAGS ?= -O2 -fomit-frame-pointer
  CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
endif



ifeq ($(USE_CSS), 1)
  CFLAGS += -DHAVE_CSS
endif

ifeq ($(USE_MMX), 1)
  CFLAGS += -DHAVE_MMX
  ASMOBJS = $(OBJDIR)/video/mmxidct.o
  NASMOBJS = $(OBJDIR)/video/reconmmx.o
endif






CFLAGS += \
	-I. \
	-I$(A52DIR)/include \
	-I$(A52DIR)/liba52



CFLAGS += -g










OBJS = \
	$(OBJDIR)/audio/ac3.o \
	$(OBJDIR)/audio/dct.o \
	$(OBJDIR)/audio/huffman.o \
	$(OBJDIR)/audio/layer2.o \
	$(OBJDIR)/audio/layer3.o \
	$(OBJDIR)/audio/mpeg3audio.o \
	$(OBJDIR)/audio/pcm.o \
	$(OBJDIR)/audio/synthesizers.o \
	$(OBJDIR)/audio/tables.o \
	$(OBJDIR)/libmpeg3.o \
	$(OBJDIR)/mpeg3atrack.o \
	$(OBJDIR)/mpeg3bits.o \
	$(OBJDIR)/mpeg3css.o \
	$(OBJDIR)/mpeg3demux.o \
	$(OBJDIR)/mpeg3ifo.o \
	$(OBJDIR)/mpeg3io.o \
	$(OBJDIR)/mpeg3strack.o \
	$(OBJDIR)/mpeg3title.o \
	$(OBJDIR)/mpeg3tocutil.o \
	$(OBJDIR)/mpeg3vtrack.o \
	$(OBJDIR)/video/getpicture.o \
	$(OBJDIR)/video/headers.o \
	$(OBJDIR)/video/idct.o \
	$(OBJDIR)/video/macroblocks.o \
	$(OBJDIR)/video/mmxtest.o \
	$(OBJDIR)/video/motion.o \
	$(OBJDIR)/video/mpeg3cache.o \
	$(OBJDIR)/video/mpeg3video.o \
	$(OBJDIR)/video/output.o \
	$(OBJDIR)/video/reconstruct.o \
	$(OBJDIR)/video/seek.o \
	$(OBJDIR)/video/slice.o \
	$(OBJDIR)/video/subtitle.o \
	$(OBJDIR)/video/vlc.o \
	$(OBJDIR)/workarounds.o

#OBJS = \
#	$(OBJDIR)/audio/ac3.o \
#	$(OBJDIR)/audio/bit_allocation.o \
#	$(OBJDIR)/audio/exponents.o \
#	$(OBJDIR)/audio/header.o \
#	$(OBJDIR)/audio/huffman.o \
#	$(OBJDIR)/audio/layer2.o \
#	$(OBJDIR)/audio/layer3.o \
#	$(OBJDIR)/audio/mantissa.o \
#	$(OBJDIR)/audio/pcm.o \
#	$(OBJDIR)/audio/tables.o \




DIRS := \
	$(OBJDIR)/audio \
	$(OBJDIR)/video

include Makefile.a52

DIRS += $(A52DIRS)


OUTPUT = $(OBJDIR)/libmpeg3.a
UTILS = $(OBJDIR)/mpeg3dump $(OBJDIR)/mpeg3peek $(OBJDIR)/mpeg3toc  $(OBJDIR)/mpeg3cat

#$(OBJDIR)/mpeg3split


LIBS = -lm -lpthread

$(shell mkdir -p $(OBJDIR) $(DIRS))

all: $(OUTPUT) $(UTILS)


$(OUTPUT): $(OBJS) $(ASMOBJS) $(NASMOBJS) $(A52OBJS)
	ar rcs $(OUTPUT) $(OBJS) $(ASMOBJS) $(A52OBJS) $(NASMOBJS)

progs += ${OBJDIR}/mpeg3dump
progs += ${OBJDIR}/mpeg3peek
progs += ${OBJDIR}/mpeg3toc
progs += ${OBJDIR}/mpeg3cat
#progs += $(OBJDIR)/mpeg3split
${OBJDIR}/mpeg3dump: mpeg3dump.c
${OBJDIR}/mpeg3peek: mpeg3peek.c
${OBJDIR}/mpeg3toc: mpeg3toc.c
${OBJDIR}/mpeg3cat: mpeg3cat.c
${OBJDIR}/mpeg3split: mpeg3split.c
${progs}:
	${CC} ${CFLAGS} -o $@ ${@F}.c ${OUTPUT} ${LIBS}

$(OBJDIR)/mpeg2qt: $(OUTPUT)
	$(CC) ${CFLAGS} -o $(OBJDIR)/mpeg2qt mpeg2qt.c \
		$(OUTPUT) \
		$(LIBS) \
		-I. \
		-I../quicktime \
		../quicktime/$(OBJDIR)/libquicktime.a \
		-lpng \
		-lz \
		-ldl

install: 
	cp $(UTILS) $(DST)

clean:
	rm -rf $(OBJDIR)

backup: clean
	cd .. && \
	tar zcf libmpeg3.tar.gz libmpeg3

wc:
	cat *.c *.h audio/*.c audio/*.h video/*.c video/*.h | wc

$(OBJS): 
	${CC} -c ${CFLAGS} $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(ASMOBJS): 
	${CC} -c ${CFLAGS} $(subst $(OBJDIR)/,, $*.S) -o $*.o

$(NASMOBJS): 
	$(NASM) -f elf $(subst $(OBJDIR)/,, $*.s) -o $*.o

$(A52OBJS):
	${CC} -c ${A52CFLAGS} $(subst $(OBJDIR)/,, $*.c) -o $*.o

$(OBJDIR)/libmpeg3.o: 				    libmpeg3.c
$(OBJDIR)/mpeg3atrack.o: 			    mpeg3atrack.c
$(OBJDIR)/mpeg3bits.o:  			    mpeg3bits.c
$(OBJDIR)/mpeg3css.o: 				    mpeg3css.c
$(OBJDIR)/mpeg3demux.o: 			    mpeg3demux.c
$(OBJDIR)/mpeg3dump.o: 				    mpeg3dump.c
$(OBJDIR)/mpeg3ifo.o: 				    mpeg3ifo.c
$(OBJDIR)/mpeg3io.o: 				    mpeg3io.c
$(OBJDIR)/mpeg3strack.o:                            mpeg3strack.c
$(OBJDIR)/mpeg3title.o: 			    mpeg3title.c
$(OBJDIR)/mpeg3toc3.o:  			    mpeg3toc3.c
$(OBJDIR)/mpeg3toc.o: 				    mpeg3toc.c
$(OBJDIR)/mpeg3tocutil.o:                           mpeg3tocutil.c
$(OBJDIR)/mpeg3vtrack.o: 			    mpeg3vtrack.c
$(OBJDIR)/audio/ac3.o:  			    audio/ac3.c
$(OBJDIR)/audio/bit_allocation.o: 		    audio/bit_allocation.c
$(OBJDIR)/audio/dct.o:  			    audio/dct.c
$(OBJDIR)/audio/exponents.o: 			    audio/exponents.c
$(OBJDIR)/audio/header.o: 			    audio/header.c
$(OBJDIR)/audio/huffman.o: 			    audio/huffman.c
$(OBJDIR)/audio/layer2.o: 			    audio/layer2.c
$(OBJDIR)/audio/layer3.o: 			    audio/layer3.c
$(OBJDIR)/audio/mantissa.o: 			    audio/mantissa.c
$(OBJDIR)/audio/mpeg3audio.o: 			    audio/mpeg3audio.c
$(OBJDIR)/audio/pcm.o:  			    audio/pcm.c
$(OBJDIR)/audio/synthesizers.o: 		    audio/synthesizers.c
$(OBJDIR)/audio/tables.o: 			    audio/tables.c
$(OBJDIR)/video/getpicture.o: 			    video/getpicture.c
$(OBJDIR)/video/headers.o: 			    video/headers.c
$(OBJDIR)/video/idct.o: 			    video/idct.c
$(OBJDIR)/video/macroblocks.o:  		    video/macroblocks.c
$(OBJDIR)/video/mmxtest.o: 			    video/mmxtest.c
$(OBJDIR)/video/motion.o: 			    video/motion.c
$(OBJDIR)/video/mpeg3cache.o: 			    video/mpeg3cache.c
$(OBJDIR)/video/mpeg3video.o: 			    video/mpeg3video.c
$(OBJDIR)/video/output.o: 			    video/output.c
$(OBJDIR)/video/reconstruct.o:  		    video/reconstruct.c
$(OBJDIR)/video/seek.o: 			    video/seek.c
$(OBJDIR)/video/slice.o: 			    video/slice.c
$(OBJDIR)/video/subtitle.o:                         video/subtitle.c
$(OBJDIR)/video/vlc.o:  			    video/vlc.c
$(OBJDIR)/workarounds.o:  			    workarounds.c



include depend.a52
