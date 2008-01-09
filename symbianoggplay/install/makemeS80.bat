call cd ..\groupS80
call abld build armi urel

call cd ..\install
call makesis -dC:\Symbian\7.0s\S80_DP2_0_SDK OggPlayMMF.s80.pkg
