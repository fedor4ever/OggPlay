call cd ..\groupS60V3MMF
call abld build gcce urel

call cd ..\install
call makesis -dC:\Symbian\9.1\S60_3rd_MR OggPlay.s60V3MMF.pkg
call signsis OggPlay.S60V3MMF.sis OggPlay.sis oggplay.cer oggplay.key OggPlay
