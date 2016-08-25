mkdir backup

move ptlibd_2010.dtf backup\
copy ..\..\..\lib\debug\ptlibd.def ptlibd_2010.dtf
move ptlib_2010.dtf backup\
copy ..\..\..\lib\release\ptlib.def ptlib_2010.dtf
move ptlibn_2010.dtf backup\
copy "..\..\..\lib\No Trace\ptlibn.def" ptlibn_2010.dtf

rem move ptlibd_2010_wm.dtf backup\
rem copy ..\..\..\lib\wm5ppc\debug\ptlibd.def ptlibd_2010_wm.dtf
rem move ptlib_2010_wm.dtf backup\
rem copy ..\..\..\lib\wm5ppc\release\ptlib.def ptlib_2010_wm.dtf
rem move ptlibn_2010_wm.dtf backup\
rem copy "..\..\..\lib\wm5ppc\No Trace\ptlibn.def" ptlibn_2010_wm.dtf

rem move ptlibd_2010_wm6.dtf backup\
rem copy ..\..\..\lib\wm6pro\debug\ptlibd.def ptlibd_2010_wm6.dtf
rem move ptlib_2010_wm6.dtf backup\
rem copy ..\..\..\lib\wm6pro\release\ptlib.def ptlib_2010_wm6.dtf
rem move ptlibn_2010_wm6.dtf backup\
rem copy "..\..\..\lib\wm6pro\No Trace\ptlibn.def" ptlibn_2010_wm6.dtf

pause