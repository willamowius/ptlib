mkdir backup

move ptlibd_2008.dtf backup\
copy ..\..\..\lib\debug\ptlibd.def ptlibd_2008.dtf
move ptlib_2008.dtf backup\
copy ..\..\..\lib\release\ptlib.def ptlib_2008.dtf
move ptlibn_2008.dtf backup\
copy "..\..\..\lib\No Trace\ptlibn.def" ptlibn_2008.dtf

rem move ptlibd_2008_wm.dtf backup\
rem copy ..\..\..\lib\wm5ppc\debug\ptlibd.def ptlibd_2008_wm.dtf
rem move ptlib_2008_wm.dtf backup\
rem copy ..\..\..\lib\wm5ppc\release\ptlib.def ptlib_2008_wm.dtf
rem move ptlibn_2008_wm.dtf backup\
rem copy "..\..\..\lib\wm5ppc\No Trace\ptlibn.def" ptlibn_2008_wm.dtf

rem move ptlibd_2008_wm6.dtf backup\
rem copy ..\..\..\lib\wm6pro\debug\ptlibd.def ptlibd_2008_wm6.dtf
rem move ptlib_2008_wm6.dtf backup\
rem copy ..\..\..\lib\wm6pro\release\ptlib.def ptlib_2008_wm6.dtf
rem move ptlibn_2008_wm6.dtf backup\
rem copy "..\..\..\lib\wm6pro\No Trace\ptlibn.def" ptlibn_2008_wm6.dtf

pause