call del *.mbm
call del *.sis
call bmconv AnalyzerTest.cmd
call makesis AnalyzerTest.pkg
copy *.mbm \epoc32\release\wins\udeb\z\system\apps\oggplay\
copy *.skn \epoc32\release\wins\udeb\z\system\apps\oggplay\
pause
