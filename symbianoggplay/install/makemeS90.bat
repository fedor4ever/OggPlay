call cd ..\groupS90
call abld build armi urel

call cd ..\install
call makesis -dC:\Symbian\7.0s\Nokia_7710_SDK OggPlayMMF.s90.pkg
