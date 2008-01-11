call cd ..\bitmaps\Skins\mcNuOggPlay
call bmconv mcNuOggPlay.cmd

call cd ..\S60V3\mcNuOggPlayDR
call bmconv mcNuOggPlayDR.cmd

call cd ..\mcNuOggPlayQVGA
call bmconv mcNuOggPlayQVGA.cmd

call cd ..\mcNuOggPlayLR
call bmconv mcNuOggPlayLR.cmd

call cd ..\mcNuOggPlaySQ
call bmconv mcNuOggPlaySQ.cmd


call cd ..\..\ClearS80\source
call ClearS80_CreateMBM.bat

call cd ..\..\icyS80
call bmconv icys80.cmd

call cd ..\icyS90
call bmconv icys90.cmd


call cd ..\..\..\install
