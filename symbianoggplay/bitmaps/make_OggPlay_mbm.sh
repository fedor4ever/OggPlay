# This shell script creates the multi bitmap file OggPlay.mbm
# (and the mbm file used for the aif file).
#
# By convention the first 18 bitmaps must be the icons for the playlist (bitmap+mask each).
#
#  0 title
#  1 title mask
#
#  2 album
#  3 album mask
#
#  4 artist
#  5 artist mask
#
#  6 genre
#  7 genre mask
#
#  8 subfolder
#  9 subfolder mask
#
# 10 file 
# 11 file mask
#
# 12 "back" icon
# 13 "back" icon mask
#
# 14 playing
# 15 playing mask
#
# 16 paused
# 17 paused mask
#
# The next two bitmaps must be the background images for flip-closed and flip-open mode:
#
# 18 background image flip-closed
# 19 background image flip-open
#
# The remainig bitmaps are not fixed and can be different for each skin.
# Following are the bitmaps used by the default skin:
#
# 20 volume slider
# 21 volume slider mask
#
# 22 a closed eye (used by the fish)
#
# 23 repeat icon
# 24 repeat icon mask
#
# 25 song position slider
# 26 song position slider mask
#
# 27 scrollbar knob
# 28 scrollbar knob mask
#
# 29 analyzer bar
# 30 analyzer bar mask
#
# 31 stop icon
# 32 stop icon mask
# 33 stop icon dimmed
#
# 34 play icon
# 35 play icon mask
# 36 play icon dimmed
# 
# 37 pause icon
# 38 pause icon mask
#
# 39 alarm
# 40 alarm mask
#
# 41 OggPlay Logo

wine ~/p800/gcc/Tools/BMCONV.EXE OggPlay.mbm /c12title.bmp /1titlem.bmp /c12cdicon.bmp /1cdiconm.bmp /c12micicon.bmp /1miciconm.bmp /c12genre.bmp /1genrem.bmp /c12folder.bmp /1folderm.bmp /c12file.bmp /1filem.bmp /c12back.bmp /1backm.bmp /c12playing.bmp /1playingm.bmp /c12paused.bmp /1pausedm.bmp /c12flipclosed.bmp /c12flipopen_steel.bmp /c12volume.bmp /1volumem.bmp /1eyeclosed.bmp /c12repeat.bmp /1repeatm.bmp /c12slider.bmp /1sliderm.bmp /c12knob.bmp /1knobm.bmp /c12bar.bmp /1barm.bmp /c12stop.bmp /1stopm.bmp /1stop_dimmed.bmp /c12play.bmp /1playm.bmp /1play_dimmed.bmp /c12pause.bmp /1pausem.bmp /c12alarm.bmp /1alarmm.bmp

wine ~/p800/gcc/Tools/BMCONV.EXE Symbian.mbm /c12title.bmp /1titlem.bmp /c12cdicon.bmp /1cdiconm.bmp /c12micicon.bmp /1miciconm.bmp /c12genre.bmp /1genrem.bmp /c12folder.bmp /1folderm.bmp /c12file.bmp /1filem.bmp /c12back.bmp /1backm.bmp /c12playing.bmp /1playingm.bmp /c12paused.bmp /1pausedm.bmp /c12symbian/flipclosed.bmp /c12symbian/flipopen.bmp /c12volume.bmp /1volumem.bmp /1eyeclosed.bmp /c12repeat.bmp /1repeatm.bmp /c12symbian/slider.bmp /1symbian/sliderm.bmp /c12knob.bmp /1knobm.bmp /c12bar.bmp /1barm.bmp /c12stop.bmp /1stopm.bmp /1stop_dimmed.bmp /c12play.bmp /1playm.bmp /1play_dimmed.bmp /c12pause.bmp /1pausem.bmp /c12alarm.bmp /1alarmm.bmp /c12symbian/OggPlayLogo.bmp

# mbm file used for creating the aif file:
wine ~/p800/gcc/Tools/BMCONV.EXE OggPlayAif.mbm /c12cdicon.bmp /1cdiconm.bmp
