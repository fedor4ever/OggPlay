#
#  Copyright (c) 2003 L. H. Wilden.
# 
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
# 
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
# 
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
# 

VPATH=data src inc aif bitmaps

EPOC=/home/wilden/p800/gcc/
PATH=$(EPOC)/bin:/local/bin:/usr/bin:/bin:/usr/local/bin
include eikon

LIBS=	$(EPOCTRGREL)/euser.lib \
	$(EPOCTRGREL)/apparc.lib \
	$(EPOCTRGREL)/cone.lib \
	$(EPOCTRGREL)/efsrv.lib \
	$(EPOCTRGREL)/eikcore.lib \
	$(EPOCTRGREL)/eikdlg.lib \
	$(EPOCTRGREL)/eikcoctl.lib \
	$(EPOCTRGREL)/ws32.lib \
	$(EPOCTRGREL)/bafl.lib \
	$(EPOCTRGREL)/apgrfx.lib \
	$(EPOCTRGREL)/egul.lib \
	$(EPOCTRGREL)/fbscli.lib \
	$(EPOCTRGREL)/bitgdi.lib \
	$(EPOCTRGREL)/gdi.lib \
	$(EPOCTRGREL)/hal.lib \
	$(EPOCTRGREL)/eikctl.lib \
	$(EPOCTRGREL)/qikctl.lib \
	$(EPOCTRGREL)/qikdlg.lib \
	$(EPOCTRGREL)/qikutils.lib \
	$(EPOCTRGREL)/mediaclientaudiostream.lib \
	$(EPOCTRGREL)/estlib.lib \
	$(EPOCTRGREL)/etel.lib \
	$(EPOCTRGREL)/charconv.lib \
	$(EPOCTRGREL)/mediaclientaudio.lib \
	$(EPOCTRGREL)/mediaclientimage.lib \
	$(EPOCTRGREL)/flogger.lib

NAME=OggPlay
OBJECTS=src/OggPlay.o src/OggPlayAppView.o src/OggDialogs.o src/OggTremor.o src/OggViews.o src/OggControls.o src/OggFiles.o vorbis/window.o vorbis/synthesis.o vorbis/bitwise.o vorbis/block.o vorbis/sharedbook.o vorbis/registry.o vorbis/floor0.o vorbis/floor1.o vorbis/codebook.o vorbis/res012.o vorbis/mapping0.o vorbis/mdct.o vorbis/ivorbisfile.o vorbis/framing.o vorbis/info.o src/OggLog.o
TARGET=$(NAME).app
PKGVERS=0,97

PKGFILES=$(NAME).app $(NAME).aif $(NAME).rsc $(NAME).mbm

U1 = 1000007a
U2 = 100039ce 
U3 = 1000A661

CFLAGS = -O -I. -Iinc -Idata -DUID3=0x$(U3) -I$(EPOC)/include/libc -Ivorbis -D_P800 -D_EPOC32_70 -D_ARM_ASSEM_ -Wall -D_LOW_ACCURACY_
CPPFLAGS += -D_P800 -D_EPOC32_70 -D_ARM_ASSEM_ -D_LOW_ACCURACY_

all: $(PKGFILES) $(NAME).sis

$(NAME).sis: $(NAME).app data/$(NAME).rsc data/$(NAME).ge.rsc data/$(NAME).fr.rsc data/$(NAME).it.rsc data/$(NAME).tu.rsc data/$(NAME).du.rsc data/$(NAME).sp.rsc data/$(NAME).sw.rsc $(NAME).pkg bitmaps/$(NAME).mbm

$(TARGET):$(OBJECTS)

# aif file must be generated manually (use aiftool)
#$(NAME).aifspec:
#	echo "mbmfile=rednote.mbm" > $(NAME).aifspec
#	echo "ELangEnglish=$(NAME)" >> $(NAME).aifspec

src/$(NAME).o: data/$(NAME).rsc data/$(NAME).ge.rsc data/$(NAME).fr.rsc data/$(NAME).it.rsc data/$(NAME).tu.rsc data/$(NAME).du.rsc data/$(NAME).sp.rsc data/$(NAME).sw.rsc $(NAME).h

clean:
	rm -f $(GENERATED) $(NAME).aifspec vorbis/*.o
	rm -f $(NAME).aif $(NAME).rsc $(NAME).mbm
	cd data && rm -f $(GENERATED)
	cd src && rm -f $(GENERATED)
