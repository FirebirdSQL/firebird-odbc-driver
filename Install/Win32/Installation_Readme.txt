The IBPhoenix Firebird ODBC Driver Installation
===============================================

The installer presents 3 installation options:

o Developer Install
o Deployment Install
o Documentation Install

General Notes on installing the Driver
--------------------------------------

ODBC Drivers live in the WINDOWS System32 ( <sys> ) directory. When the
installer prompts you to choose an installation directory it is really
asking you where you want the documentation installed.

All options are uninstallable from the control panel.


Developer Install
-----------------

This option will install the driver into the <sys> directory and register it.
It will also install the documentation into the chosen installation directory.
As the name suggests, this is the recommended option if you are developing
applications with the driver.


Deployment Install
------------------

This option will install the driver into the <sys>
directory and register it. No documentation will be installed into the
installation directory. The online help for DSN configuration is installed,
because it is stored in the System32 directory with the driver itself. The
installation directory will be deleted after install.

This option is recommended if you are deploying the driver with your
application.


Documentation Install
---------------------

This just installs documentation into the chosen installation directory.



Installation from a batch file
------------------------------

The setup program can be run from a batch file. The
following parameters may be passed:


/SP-
  Disables the 'This will install... Do you wish to
  continue?' prompt at the beginning of Setup.

/SILENT, /VERYSILENT
  Instructs Setup to be silent or very silent. When
  Setup is silent the wizard and the background window
  are not displayed but the installation progress
  window is. When a setup is very silent this
  installation progress window is not displayed.
  Everything else is normal so for example error
  messages during installation are displayed and the
  startup prompt is (if you haven't disabled it with
  the '/SP-' command line option explained above)

  If a restart is necessary and the '/NORESTART'
  command isn't used (see below) and Setup is silent,
  it will display a Reboot now? messagebox. If it's
  very silent it will reboot without asking.

/NORESTART
  Instructs Setup not to reboot even if it's necessary.

/DIR="x:\dirname"
  Overrides the default directory name displayed on
  the Select Destination Directory wizard page. A
  fully qualified pathname must be specified. If the
  [Setup] section directive DisableDirPage was set to
  yes, this command line parameter is ignored.

/GROUP="folder name"
  Overrides the default folder name displayed on the
  Select Start Menu Folder wizard page. If the [Setup]
  section directive DisableProgramGroupPage was set to
  yes, this command line parameter is ignored.

/NOICONS
  Instructs Setup to initially disable the Don't create
  any icons check box on the Select Start Menu Folder
  wizard page.

/COMPONENTS="comma separated list of component names"

  Choose from - DeveloperComponent
  				DeploymentComponent
				DocumentationComponent

  Overrides the default components settings.
  Components cannot be combined.

  For example:

    /COMPONENTS="DeploymentComponent"




