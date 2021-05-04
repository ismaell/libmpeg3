CC = gcc
NASM = nasm
USE_CSS = 1

DEST =
prefix = /usr
bindir = ${prefix}/bin

ARCH = $(shell uname -m)
OBJDIR := ${ARCH}

ifeq (${ARCH}, alpha)
  CFLAGS ?= -O4 -arch ev67 -ieee -accept c99_keywords -gcc_messages
endif

ifeq (${ARCH}, i686)
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

i686-USE_MMX = 1
USE_MMX := ${${ARCH}-USE_MMX}
ifeq ($(USE_MMX), 1)
  CFLAGS += -DHAVE_MMX
  ASMOBJS = $(OBJDIR)/video/mmxidct.o
  NASMOBJS = $(OBJDIR)/video/reconmmx.o
endif

CFLAGS += -I.
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

OUTPUT = $(OBJDIR)/libmpeg3.a
UTILS = $(OBJDIR)/mpeg3dump $(OBJDIR)/mpeg3peek $(OBJDIR)/mpeg3toc  $(OBJDIR)/mpeg3cat

#$(OBJDIR)/mpeg3split


LIBS = -lm -lpthread -la52

$(shell mkdir -p $(OBJDIR) $(DIRS))

all: $(OUTPUT) $(UTILS)

$(OUTPUT): $(OBJS) $(ASMOBJS) $(NASMOBJS)
	ar rcs $(OUTPUT) $(OBJS) $(ASMOBJS) $(NASMOBJS)

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

.PHONY: install
install: ${UTILS}
	install -d ${DEST}${bindir}
	install -m555 ${UTILS} ${DEST}${bindir}

clean:
	rm -rf $(OBJDIR)

backup: clean
	cd .. && \
	tar zcf libmpeg3.tar.gz libmpeg3

wc:
	cat *.c *.h audio/*.c audio/*.h video/*.c video/*.h | wc

${ASMOBJS}: ${OBJDIR}/%.o: %.S
	${CC} -c ${CFLAGS} -o $@ $<

${NASMOBJS}: ${OBJDIR}/%.o: %.s
	${NASM} -f elf -o $@ $<

${OBJDIR}/%.o: %.c
	${CC} ${CFLAGS} -c -o $@ $<
