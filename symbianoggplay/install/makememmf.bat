call cd ..\groupS60MMF
call abld build armi urel

call cd ..\install
call makesis -dC:\Symbian\8.1a\S60_2nd_FP3 OggPlayMMF.s60.pkg
