================================================================================
IeSwitchProxy 0.2.3
Copyright ©2007-2008 Liam Kirton <liam@int3.ws>

2nd April 2008
http://int3.ws/
================================================================================

Overview:
---------

IeSwitchProxy is a simple toolbar for Internet Explorer that allows for rapidly
switching between pre-configured proxy setting profiles.

Requirements:
-------------

Visual C++ 2008 runtime libraries are required. If these are not installed,
they are available from:

http://www.microsoft.com/downloads/details.aspx?familyid=200b2fd9-ae1a-4a14-984d-389c36f85647&displaylang=en

Install:
--------

> Install.bat

OR

> regsvr32 x86\IeSwitchProxy.dll (For Windows x86)
> regsvr32 x64\IeSwitchProxy.dll (For Windows x64)

Uninstall:
----------

> regsvr32 /u x86\IeSwitchProxy.dll
> regsvr32 /u x64\IeSwitchProxy.dll

Usage:
------

Enable and position the toolbar within Internet Explorer.

Use the settings dialog to configure proxy profiles. Note that after filling
in the required details for a new profile, click the "Add" button to add it to
the list.

Note, add <local> to the exclusions list (which is semicolon separated) to
enable the "Bypass proxy server for local addresses" option.

Example:
--------

The creation of three proxy profiles:

Name: None
Address:
Exclusions:

Click Add

Name: Localhost
Address: localhost:8080
Exclusions:

Click Add

Name: Work Intranet
Address: proxy.intranet.work.com:8080
Exclusions: <local>;*.intranet.work.com;192.168.*

Click Add

Click Save

================================================================================