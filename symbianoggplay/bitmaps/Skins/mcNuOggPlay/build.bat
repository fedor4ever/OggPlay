call bmconv mcNuOggPlay.cmd
call bmconv s60splash.mbm /c16bitmaps/intro.bmp
call makesis mcNuOggPlay.pkg
zip dist/mcNuOggPlay.zip -D version.txt mcNuOggPlay.mbm mcNuOggPlay.skn s60splash.mbm
pause