#!/bin/bash
#
#    The contents of this file are subject to the Initial
#    Developer's Public License Version 1.0 (the "License");
#    you may not use this file except in compliance with the
#    License. You may obtain a copy of the License at
#    http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl
#
#    Software distributed under the License is distributed on
#    an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
#    express or implied.  See the License for the specific
#    language governing rights and limitations under the License.
#
#
#    Copyright (c) 2004 Paul Reeves
#    All Rights Reserved.
#
# To Do for a future version
#    Consider adding an install log
#    Make verbose output consistent
#    Consider adding --force for those that really don't care.
#    Consider adding a --status option - Does unixODBC exist. Is the driver already installed?
#
#   Test the install
#   Write uninstall docs function
#
#
#	Errors
#	------
#	3	- Help called
#   4   - More than one distribution release file was found
#           ie, it looks as if user crossed over from one distro
#           to another.
#   5   - Installation of libraries failed
#   6   - Docs directory does not exist
#   7   - A problem occurred while running odbcinst to registering or remove the driver.
#   9   - Can't find odbcinst. Is unixODBC actually installed?
#

##################################################################################
ShowHelp()
{
    echo "This script controls the installation of the                           "
    echo "Firebird ODBC driver for Linux.                                        "
    echo "                                                                       "
    echo "Usage:                                                                 "
    echo "No parameters are required for installation.                           "
    echo "                                                                       "
    echo "  `basename $0`  [option ..]                                           "
    echo "  `basename $0`  --help                                                "
    echo "                                                                       "
    echo "Command line Options:                                                  "
    echo "                                                                       "
    echo "  --libs=/path/to/libs    Override default location of             OPT."
    echo "                          driver libraries.                            "
    echo "                          /usr/lib/unixODBC is assumed by default.     "
    echo "  --docs=/path/to/docs    Override default location of docs.       OPT."
    echo "  --createTestDSN         Create a test DSN to example employee    OPT."
    echo "                          database using default SYSDBA/password       "
    echo "  --uninstall             Uninstall the driver                     OPT."
    echo "                                                                       "
    echo "  --test                  Test script but do nothing               OPT."
    echo "  --verbose               Log more info.                           OPT."
    echo "  --help                  This help screen                             "
    echo "                                                                       "

    exit 3
}

##################################################################################
ErrorHelp()
{
    if [ -n "$1" ]; then
    errcode=$1
    else
    errcode="99"
    fi

    echo ""
    case $errcode in
        4)
            echo "More than one Distribution Release file found."
            ;;
        5)
            echo "Tar error. Installation terminating"
            ;;
        6)
            echo "  The Default documentation root directory does not exist."
            echo "  You can try running the install script with    "
            echo "    --docs=/path/to/docs/on/your/distribution"
            echo "  as a parameter.    "
            echo ""
            echo "  The Firebird ODBC Driver documentation will be created "
            echo "  in a sub-directory under the directory specified."
            ;;
        7)
            echo "  A problem occurred while running odbcinst to "
            echo "  register or remove the driver."
            ;;
        9)
            echo "  Cannot find odbcinst. It appears that unixODBC "
            echo "  may not be installed. If it is installed please"
            echo "  ensure that the odbcinst program is available on "
            echo "  your path."
            ;;
        *)
            echo "  An unknown error $errcode occurred."
            ;;
    esac
    echo ""

    if [ "$TEST" != "1" ]; then
        exit $errcode
    else
        echo ""
        echo "In test mode. Continuing execution."
        echo ""
    fi

}

##################################################################################
EvaluateCommandLine()
{
    for Option in $*
    do
        OptionValue=`echo "$Option" | cut -d'=' -f2`
        OptionName=`echo "$Option" | cut -d'=' -f1`
        case $OptionName in
            --help)
                ShowHelp
                ;;
            --test)
                TEST="1" ;;
            --libs)
                fbODBCLIBS=$OptionValue ;;
            --docs)
                fbODBCDOCS=$OptionValue ;;
            --createTestDSN)
                TESTDSN="1" ;;
            --debug)
                set -x
                ;;
            --verbose)
                VERBOSE="1" ;;
            --uninstall)
                UNINSTALL="1" ;;
            *)
                echo "**** Unknown option $Option ****" 1>&2
                ShowHelp
                ;;
        esac
    done
}


##################################################################################
CheckEnvironment()
{
    if [ "$VERBOSE" = "1" ]; then
        echo Starting $FUNCNAME
    fi

    #First, check driver manager is installed.
    #If it isn't we will have a problem registering
    #the driver as the odbcinst.ini is distro specific.
    #So, might as well bail out now if we can't find odbcinst.
    odbcinst 2>/dev/null >/dev/null 
    if [ $? -eq 127 ]; then ErrorHelp 9; fi

    #Which Distro is installed?
    # We need to know this for the location of documentation and uninstall script
    DISTRO=""
    for afile in `ls /etc/*release`
    do
        DISTRO=`head -1 $afile | cut -d" " -f1 -s`
    	case "$DISTRO" in
        S[Uu]SE )
            fbODBCDOCS="/usr/share/doc/packages"
            break
            ;;
        *)
            fbODBCDOCS="/usr/share/doc"
            ;;
    	esac
    done

    if [ -z "$fbODBCDOCS" ]; then
        fbODBCDOCS="/usr/share/doc"
    fi

    if [ ! -d $fbODBCDOCS ]; then
        ErrorHelp 6
    else
        fbODBCDOCS=$fbODBCDOCS/FirebirdODBC
    fi

    #Location where we will install the driver libraries
    if [ -z "$fbODBCLIBS" ]; then
        #SuSE, at least, install drivers to a subdir
        #Let's use it if we can find it.
        if [ -d /usr/lib/unixODBC ]; then
            fbODBCLIBS="/usr/lib/unixODBC"
        else
            fbODBCLIBS="/usr/lib"
        fi
    fi
}


##################################################################################
InstallLibraries()
{
    if [ "$TEST" = "1" ]; then
        echo "Testing extraction of tarfile OdbcJdbcLibs.tar"
        tar --directory $fbODBCLIBS -tvf OdbcJdbcLibs.tar
    else
        echo "Untarring OdbcJdbcLibs.tar"
        tar --directory $fbODBCLIBS -xvf OdbcJdbcLibs.tar
#        ln -f -s $fbODBCLIBS/libIscDbc.so $fbODBCLIBS/libIscDbc
#        ln -f -s $fbODBCLIBS/libIscDbc.so $fbODBCLIBS/IscDbc
    fi

    if [ $? -ne 0 ]; then
        ErrorHelp 5
    fi
}


##################################################################################
UninstallLibraries()
{
    if [ "TEST" = "1" ]; then
        echo $FUNCNAME
    fi

    for afile in libOdbcFb.so
    do
        if [ "TEST" = "1" ]; then
            echo "In Test Mode. Would remove $(fbODBCLIBS)/$afile"
        else
            rm $fbODBCLIBS/$afile
        fi
    done
}

##################################################################################
UninstallDocs()
{
    if [ "$TEST" = "1" ]; then
        echo $FUNCNAME
    fi

    if [ "TEST" = "1" ]; then
        echo "In Test Mode. Would remove  $fbODBCDOCS/*"
    else
        rm $fbODBCDOCS/html/*
		rmdir $fbODBCDOCS/html
        rm $fbODBCDOCS/*
		rmdir $fbODBCDOCS
    fi
}

##################################################################################
InstallDocumentation()
{


if [ "$TEST" = "1" ]; then
    echo $FUNCNAME
    echo "Testing for loop that copies readme etc to $fbODBCDOCS"
    for afile in readme.txt *.ini
    do
        echo "Would copy $afile to $fbODBCDOCS"
    done

    echo "Testing extraction of tarfile OdbcJdbcDocs.tar"
    tar --directory $fbODBCDOCS -tvf OdbcJdbcDocs.tar
else
    if [ ! -d $fbODBCDOCS  ]; then
        mkdir $fbODBCDOCS
    fi

    for afile in readme.txt *.ini
    do
        cp $afile $fbODBCDOCS
    done

    echo "Untarring OdbcJdbcDocs.tar.gz"
    tar --directory $fbODBCDOCS -xvf OdbcJdbcDocs.tar
fi

if [ $? -ne 0 ]; then
    ErrorHelp 5
fi

}


##################################################################################
WriteUninstallScript()
{
if [ "$TEST" = "1" ]; then
    echo $FUNCNAME
    echo "Test Mode. This is the uninstall script that would be created"
    echo "in $fbODBCDOCS : "
    echo "  #!/bin/bash"
    echo "  ./install.sh --uninstall --libs=$fbODBCLIBS --docs=$fbODBCDOCS"
    echo ""
else
    echo "#!/bin/bash" > $fbODBCDOCS/uninstall.sh
    echo "./install.sh --uninstall --libs=$fbODBCLIBS --docs=$fbODBCDOCS" >> $fbODBCDOCS/uninstall.sh
    echo "" >> $fbODBCDOCS/uninstall.sh
    chmod 744 $fbODBCDOCS/uninstall.sh
fi
}


##################################################################################
ConfigureDriver()
{
# We need to test for existing ODBC install and update the
# odbcinst.ini files
if [ "$TEST" = "1" ]; then
    echo $FUNCNAME
    echo "In Test Mode. Querying driver status only."
    odbcinst -q -d -n Firebird || ErrorHelp 7
else
    odbcinst -i -d -f DriverTemplate.ini || ErrorHelp 7
fi
}


##################################################################################
RemoveDriver()
{
if [ "$TEST" = "1" ]; then
    echo $FUNCNAME
    echo "In Test Mode. Querying driver installation status."
    odbcinst -q -d -n Firebird || ErrorHelp 7
else
    odbcinst -u -d -n Firebird || ErrorHelp 7
fi

}


##################################################################################
CreateTestDSN()
{

if [ "$TEST" = "1" ]; then
    echo $FUNCNAME
    echo "In Test Mode. Querying DSN status."
    odbcinst -q -s -n TestFirebirdConnection
    odbcinst -q -s -n TestInterBaseConnection
else
    if [ -d opt/interbase ]; then
        odbcinst -i -s -f InterBaseDSNTemplate.ini -l
    fi

    if [ -d opt/firebird ]; then
        odbcinst -i -s -f FirebirdDSNTemplate.ini -l
    fi
fi
}


##################################################################################
CopyInstallScript()
{
if [ "$TEST" = "1" ]; then
    echo $FUNCNAME
    echo "Test Mode. Not copying install script to $fbODBCDOCS"
else
    cp install.sh $fbODBCDOCS
fi
}


############## Main #################
main(){
    EvaluateCommandLine $*
    CheckEnvironment

    if [ "$UNINSTALL" = "1" ]; then
        UninstallLibraries
        RemoveDriver
        UninstallDocs
    else
        InstallLibraries
        InstallDocumentation
        CopyInstallScript
        WriteUninstallScript
        ConfigureDriver
        if [ "$TESTDSN" = "1" ]; then
            CreateTestDSN
        fi
		echo -e \
"\n   Installation complete. \n\n \
  It is recommended that you review the readme.txt, \n \
  the release notes and the documentation in the \n \
  html sub-directory. These have been installed \n \
  in $fbODBCDOCS"
		echo ""

		echo -e \
"Installation complete. \n\nIt is recommended that you review the readme.txt, \
the release notes and the documentation in the html sub-directory. These have been installed \
in $fbODBCDOCS.\n\n\nThe Firebird ODBC driver development team" | mail -s "Firebird ODBC Driver Installation" $USER@localhost
	fi
}


main $*

