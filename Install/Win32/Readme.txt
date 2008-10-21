Firebird ODBC Driver v2.0 Readme
==========================================

o What's new
o Installation
o Configuration
o Known Issues
o Feedback


What's New
----------

Welcome to the latest release of the Firebird ODBC driver. This release
sees many significant advances in the driver. Notable changes are:

o Increased conformance to the different ODBC specifications.
o Better documentation.
o Improved installation (and uninstallation) routines for Win32 and Linux.


All the new features and fixes are documented in the release notes.


o Installation
--------------

Just click through the binary executable for a default install. More information
is available in the Installation Readme, which viewable within the installation
process.


o Configuration
---------------

Database connections are configured from the Database Administrator
applet in the Control Panel.


o Known Issues
--------------

Applications such as OpenOffice Quickstarter retain a lock on the driver 
libraries. Installation and Uninstallation will fail under these circumstances. 
The only work around is to ensure that all applications that might be
using the driver are closed before you start installation.

There are no other known issues at this time.


Feedback
--------

If you have feedback (good or bad) please email the Firebird
OBDC driver development list at sourceforge. You can subscribe
by visiting:

  http://firebird.sourceforge.net/index.php?op=lists

There is also information on how to access this list via a
newsgroup mirror.

NOTE: This is not a support forum! It is for development issues only.
This means that general questions about ODBC, SQL, or database access
will probably be ignored.





