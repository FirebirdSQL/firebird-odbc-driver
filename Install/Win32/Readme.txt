This is the README file for OdbcJdbc.

You have ideas?
To you here:  
     http://www.firebirdsql.org/index.php?op=lists#fb-odbc-devel

Examples connection:
  1)Open("DSN=mcsAddress;")
  2)Open("DSN=mcsAddress;UID=MCSSITE;PWD=mcssite;")
  3)Open("DSN=mcsAddress;UID=MCSSITE;PWD=mcssite;DBNAME=172.17.2.10:/usr/local/efldata/mcsAddress.fdb;")
  4)Open("DRIVER=Firebird/InterBase(r) driver;DBNAME=172.17.2.10:/usr/local/efldata/mcsAddress.fdb;")
  5)Open("DRIVER=Firebird/InterBase(r) driver;UID=MCSSITE;PWD=mcssite;DBNAME=172.17.2.10:/usr/local/efldata/mcsAddress.fdb;")

Is allowed to use ISC_PASSWORD and ISC_USER from environment (autoexec.bat Win32)