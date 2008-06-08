@echo off

rem
rem  Examples Win98/Me
rem          build C:\Borland\BCC55 WIN98
rem
rem
rem  Examples Win XP/2000/2003/...
rem          build C:\Borland\BCC55
rem


%1\bin\make -f makefile.bcc55 COMPDIR=%1 VER_NT=%2

@echo on
