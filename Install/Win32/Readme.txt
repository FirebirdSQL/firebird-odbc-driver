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

=======================================================================
2) Write: Jorge Andres Brugger
   Sent: Wednesday, May 12, 2004 10:13 PM
=======================================================================
HOW TO USE FIREBIRD WITH MIXED CASE OBJECT NAMES AND CLARION 
(including the import from the DB to the Clarion dictionary)

Version 1.1

1. Create your database in Firebird. You can use table names like 
"Pending_Invoices" and fields like "Order_Number".
2. Install 1.02.0060 or newer version of the IBPhoenix ODBC driver. I 
suggest to download the newest version at http://www.praktik.km.ua/
3. Create the DSN for the Database, making sure to check all options in 
"Extend Property Identifier"
4. Open your dictionary, and normally import multiple tables from the 
odbc source. It will work, but do not try to browse or use the files in 
a app yet.
5. For every field, type in the 'External Name' the name of the field 
surrounded by quotes (in my example, type "Order_Number" in the external 
name).
6. It?s done! Now use your dictionary with Mixed_Case_Identificators 
without problem. Just for note, remember to use the quotes also in sql 
sentences from inside Clarion.

Thanks to Vernon Godwin and Vladimir Tsvigun for the info contained in 
this how-to.

=======================================================================
