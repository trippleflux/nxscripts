################################################################################
#          nxTools - Dupe Checker, nuker, pre, request and utilities.          #
#                     Written by neoxed (neoxed@gmail.com)                     #
################################################################################

Topics:
 1. Information
 2. Requirements
 3. Installation
 4. Commands
 5. Text Templates
 6. Todo List
 7. Bugs and Comments
 8. License

################################################################################
# 1. Information                                                               #
################################################################################

    nxTools is a fully-featured ioFTPD script. Such features include: a dupe
checker, nuker, pre script, request script, and various other tools and various
statistical commands. Written in Tcl, which offers fast processing speed and
easily customizable output.


################################################################################
# 2. Requirements                                                              #
################################################################################

    nxTools should only be installed on Windows NT/2000/XP/2003 any other
operating system is unsupported. This script was tested on Beta-v5.8.5r so I
cannot guarantee it will work on any other versions of ioFTPD.


################################################################################
# 3. Installation                                                              #
################################################################################

 1. UnRAR and copy all files and directories, and place the files as follows:

    ioFTPD\scripts\init.itcl
    ioFTPD\scripts\nxLib.tcl
    ioFTPD\scripts\nxTools\data\*
    ioFTPD\scripts\nxTools\text\*
    ioFTPD\scripts\nxTools\nxClose.tcl
    ioFTPD\scripts\nxTools\nxDB.tcl
    ioFTPD\scripts\nxTools\nxDupe.tcl
    ioFTPD\scripts\nxTools\nxNuke.tcl
    ioFTPD\scripts\nxTools\nxPre.cfg
    ioFTPD\scripts\nxTools\nxPre.tcl
    ioFTPD\scripts\nxTools\nxRequest.tcl
    ioFTPD\scripts\nxTools\nxRules.cfg
    ioFTPD\scripts\nxTools\nxRules.tcl
    ioFTPD\scripts\nxTools\nxTools.cfg
    ioFTPD\scripts\nxTools\nxUtilities.tcl
    ioFTPD\scripts\nxTools\nxWeekly.cfg
    ioFTPD\system\nxHelper.dll
    ioFTPD\system\tclsqlite3.dll

 2. Modify the configuration files to your liking.

    nxTools.cfg  - Main nxTools configuration.
    nxPre.cfg    - Pre areas and groups, managed by "SITE EDITPRE".
    nxRules.cfg  - Rules displayed on "SITE RULES".
    nxWeekly.cfg - Weekly user and group credits, managed by "SITE WEEKLY".

 3. Add the following to your ioFTPD.ini

[Events]
OnUploadComplete = TCL ..\scripts\nxTools\nxDupe.tcl UPLOAD
OnUploadError    = TCL ..\scripts\nxTools\nxDupe.tcl UPLOADERROR
OnFtpLogIn       = TCL ..\scripts\nxTools\nxUtilities.tcl ONELINES

[FTP_Pre-Command_Events]
mkd  = TCL ..\scripts\nxTools\nxDupe.tcl PREMKD
stor = TCL ..\scripts\nxTools\nxDupe.tcl PRESTOR
pass = TCL ..\scripts\nxTools\nxClose.tcl LOGIN

[FTP_Post-Command_Events]
dele = TCL ..\scripts\nxTools\nxDupe.tcl DUPELOG
mkd  = TCL ..\scripts\nxTools\nxDupe.tcl POSTMKD
rmd  = TCL ..\scripts\nxTools\nxDupe.tcl DUPELOG
rnfr = TCL ..\scripts\nxTools\nxDupe.tcl DUPELOG
rnto = TCL ..\scripts\nxTools\nxDupe.tcl DUPELOG

[FTP_Custom_Commands]
close       = TCL ..\scripts\nxTools\nxClose.tcl CLOSE
open        = TCL ..\scripts\nxTools\nxClose.tcl OPEN
db          = TCL ..\scripts\nxTools\nxDB.tcl
approve     = TCL ..\scripts\nxTools\nxDupe.tcl APPROVE
clean       = TCL ..\scripts\nxTools\nxDupe.tcl CLEAN
dupe        = TCL ..\scripts\nxTools\nxDupe.tcl DUPE
fdupe       = TCL ..\scripts\nxTools\nxDupe.tcl FDUPE
new         = TCL ..\scripts\nxTools\nxDupe.tcl NEW
rebuild     = TCL ..\scripts\nxTools\nxDupe.tcl REBUILD
search      = TCL ..\scripts\nxTools\nxDupe.tcl SEARCH
undupe      = TCL ..\scripts\nxTools\nxDupe.tcl UNDUPE
wipe        = TCL ..\scripts\nxTools\nxDupe.tcl WIPE
nuke        = TCL ..\scripts\nxTools\nxNuke.tcl NUKE
nukes       = TCL ..\scripts\nxTools\nxNuke.tcl NUKES
nuketop     = TCL ..\scripts\nxTools\nxNuke.tcl NUKETOP
unnuke      = TCL ..\scripts\nxTools\nxNuke.tcl UNNUKE
unnukes     = TCL ..\scripts\nxTools\nxNuke.tcl UNNUKES
editpre     = TCL ..\scripts\nxTools\nxPre.tcl EDIT
pre         = TCL ..\scripts\nxTools\nxPre.tcl PRE
reqbot      = TCL ..\scripts\nxTools\nxRequest.tcl BOT
reqdel      = TCL ..\scripts\nxTools\nxRequest.tcl DEL
reqfill     = TCL ..\scripts\nxTools\nxRequest.tcl FILL
request     = TCL ..\scripts\nxTools\nxRequest.tcl ADD
requests    = TCL ..\scripts\nxTools\nxRequest.tcl LIST
reqwipe     = TCL ..\scripts\nxTools\nxRequest.tcl WIPE
rules       = TCL ..\scripts\nxTools\nxRules.tcl
drives      = TCL ..\scripts\nxTools\nxUtilities.tcl DRIVES
errlog      = TCL ..\scripts\nxTools\nxUtilities.tcl ERRLOG
ginfo       = TCL ..\scripts\nxTools\nxUtilities.tcl GINFO
give        = TCL ..\scripts\nxTools\nxUtilities.tcl GIVE
newdate     = TCL ..\scripts\nxTools\nxUtilities.tcl NEWDATE
onel        = TCL ..\scripts\nxTools\nxUtilities.tcl ONELINES
resetstats  = TCL ..\scripts\nxTools\nxUtilities.tcl RESETSTATS
resetuser   = TCL ..\scripts\nxTools\nxUtilities.tcl RESETUSER
rotatelogs  = TCL ..\scripts\nxTools\nxUtilities.tcl ROTATE
size        = TCL ..\scripts\nxTools\nxUtilities.tcl SIZE
syslog      = TCL ..\scripts\nxTools\nxUtilities.tcl SYSLOG
take        = TCL ..\scripts\nxTools\nxUtilities.tcl TAKE
traffic     = TCL ..\scripts\nxTools\nxUtilities.tcl TRAFFIC
weekly      = TCL ..\scripts\nxTools\nxUtilities.tcl WEEKLY
weeklyset   = TCL ..\scripts\nxTools\nxUtilities.tcl WEEKLYSET
who         = TCL ..\scripts\nxTools\nxUtilities.tcl WHO

[FTP_SITE_Permissions]
approve     = !A *
clean       = M1
close       = M1
db          = M
drives      = M1V
dupe        = !A *
editpre     = M1
errlog      = M1
fdupe       = !A *
ginfo       = M12
give        = M1
new         = !A *
newdate     = M1V
nuke        = N
nukes       = !A *
nuketop     = !A *
onel        = !A *
open        = M1
pre         = !A *
rebuild     = M1
reqbot      = M1
reqdel      = !A *
reqfill     = !A *
request     = !A *
requests    = !A *
reqwipe     = M1
resetstats  = M1
resetuser   = M1
rotatelogs  = M1
rules       = !A *
search      = !A *
size        = M1V
syslog      = M1
take        = M1
traffic     = M1
undupe      = M1V
unnuke      = N
unnukes     = !A *
weekly      = M1
weeklyset   = M1
who         = !A *
wipe        = M1V

 4. Your [Sections] configuration in the ioFTPD.ini must be a must certain format
    in order for SITE NEW, NUKE, and PRE to function properly. You must define
    the credit section; however, the stat section is optional. All values must
    be separated by space(s) and/or tab(s). Here are a few examples of the
    required sections format.

    Valid:
    Other   = 0   /other/*
    Public  = 1 0 /pub/*
    Mirrors = 1 3 /mirror/*
    Default = 0 0 /*

    Invalid:
    Default= 0 0 /*     (There should be a space before the equal sign: "Default = 0 0 /*")

 5. There are several optional scheduled events; these events must be added
    under the [Scheduler] section in the ioFTPD.ini file. Carefully read the
    explanation of each scheduler event, or you may regret using it!

    This event removes invalid and/or old entries from your file and directory
    dupe database.
nxDupeClean = 0 0 * * TCL ..\scripts\nxTools\nxDupe.tcl CLEAN

    The rotate logs even does just that, it rotates the defined log files once
    they meet or exceed the required size. Be sure to properly configure this
    option in the nxTools.cfg file.
nxRotateLogs = 59 23 * * TCL ..\scripts\nxTools\nxUtilities.tcl ROTATE

    The newdate even creates dated directories and/or today symlinks. You can
    also specify the date area with a parameter, if you want to trigger a specific
    area. Refer to the configuration file (nxTools.cfg) for more details on this.
nxNewDate = 0 0 * * TCL ..\scripts\nxTools\nxUtilities.tcl NEWDATE

    The request wipe event wipes filled requests once they meet or exceed the
    defined max age, which is defined in the configuration. It helps to keep your
    request directory clean.
nxReqWipe = 0 * * * TCL ..\scripts\nxTools\nxRequest.tcl WIPE

    If you would like to use weekly credit allotments, this event should be
    scheduled to run weekly. However, if you would like another time based credit
    allotments (i.e. daily or bi-monthly) schedule it accordingly.
nxWeekly = 0 0 * 6 TCL ..\scripts\nxTools\nxUtilities.tcl WEEKLYSET

 6. Restart ioFTPD for the changes to take effect, rehash will NOT work.

 7. Connect to your ioFTPD server and type "SITE DB CREATE" to create the SQLite
    databases used by nxTools.

 8. You can customize the display of several commands by editing the text
    templates in the scripts\nxTools\text\ directory. The available cookies are
    explained in section 5 of this manual, "Text Templates".

 9. Several scripts can be executed from message files using ioFTPD's %[execute]
    cookie. You may add these cookies to any .ioFTPD.message or ioFTPD\text\
    files you please.

    Display all current requests:
    %[execute(TCL ..\scripts\nxTools\nxRequest.tcl)(LIST)]

    Display latest one-lines:
    %[execute(TCL ..\scripts\nxTools\nxUtilities.tcl)(ONELINES)]

    Display all rules:
    %[execute(TCL ..\scripts\nxTools\nxRules.tcl)]

    Display APPS rules:
    %[execute(TCL ..\scripts\nxTools\nxRules.tcl)(APPS)]

    Display approved releases:
    %[execute(TCL ..\scripts\nxTools\nxDupe.tcl)(APPROVE LIST)]

    Display 10 latest releases:
    %[execute(TCL ..\scripts\nxTools\nxDupe.tcl)(NEW)]

    Display 15 latest releases:
    %[execute(TCL ..\scripts\nxTools\nxDupe.tcl)(NEW -max 15)]

    Display 5 latest APPS releases:
    %[execute(TCL ..\scripts\nxTools\nxDupe.tcl)(NEW -max 5 APPS)]

10. Finished, for now at least. Be sure to keep your nxTools version up-to-date!


################################################################################
# 4. Commands                                                                  #
################################################################################

Legend:
 <parameter> = Required
 [parameter] = Optional

- SITE APPROVE <ADD/DEL/LIST> [release]
    Description:
     - Add, remove or display approved releases.
     - An approved release cannot be nuked and is exempt from dupe and pre time checks.
     - The release can be approved before or after the directory was created.
    Examples:
     - SITE APPROVE ADD Something.Cool-NX
     - SITE APPROVE DEL Something.Very.Old-NX
     - SITE APPROVE LIST

- SITE CLEAN
    Description:
     - Removes old and invalid entries in the dupe database.
    Examples:
     - SITE CLEAN

- SITE CLOSE [reason]
    Description:
     - Close the FTP, all non-exempted users are kicked from the site.
     - Only exempted users may login when the FTP is closed.
    Examples:
     - SITE CLOSE
     - SITE CLOSE Software Maintenance

- SITE DB <CHECK/CREATE/OPTIMIZE> [database]
    Description:
     - This site command is used to manage the SQLite databases used by nxTools.
     - If the database parameter is not specified, the action will be performed
       on all databases.
     - CHECK validates the databases, checking for corruption.
     - CREATE creates the databases and tables, if they do not exist.
     - OPTIMIZE will vacuum the databases (not really needed).
    Examples:
     - SITE CHECK
     - SITE CHECK DupeDirs
     - SITE CREATE
     - SITE CREATE Requests
     - SITE OPTIMIZE
     - SITE OPTIMIZE OneLines

- SITE DRIVES
    Description:
     - Displays all fixed and network drives; including the volume name,
       free space, and total space.
    Examples:
     - SITE DRIVES

- SITE DUPE [-max <limit>] <release>
    Description:
     - Search the directory dupe log for a specific release.
     - The default number of results is 10, unless -max parameter is specified.
     - The -max parameter can be specified to increase or decrease results.
    Examples:
     - SITE DUPE *Something*Cool*
     - SITE DUPE -max 30 *Something*Cool*

- SITE EDITPRE <option> <area> [value]
    Description:
     - Remotely edit the pre areas, pre groups, and pre paths.
     - For an explanation of the EDITPRE command, try SITE EDITPRE HELP.
    Examples:
     - SITE EDITPRE HELP

- SITE FDUPE [-max <limit>] <filename>
    Description:
     - Search the files dupe log for a specific file.
     - The default number of results is 10, unless -max parameter is specified.
     - The -max parameter can be specified to increase or decrease results.
    Examples:
     - SITE FDUPE some*.zip
     - SITE FDUPE -max 30 some*cool.rar

- SITE GINFO <group> [credit section]
    Description:
     - Display information about a group and the users within it.
     - The credit section parameter can be specified to show the ratios and
       statistics for a specific section.
    Examples:
     - SITE GINFO thegroup
     - SITE GINFO thegroup 4

- SITE GIVE <username> <credits> [credit section]
    Description:
     - Give credits to a user, the default section is 0.
     - A size unit can be specified for the credits parameter (KB, MB or GB).
     - The credit section parameter can be specified to take credits from a
       specific section, the default section is 0.
    Examples:
     - SITE GIVE someuser 100
     - SITE GIVE someuser 100KB 3
     - SITE GIVE someuser 500GB 3

- SITE NEW [-max <limit>] [section]
    Description:
     - Display the latest directories, along with their age and owner's.
     - The sections are read from the [Sections] array in the ioFTPD.ini.
     - The default number of results is 10, unless -max parameter is specified.
     - The -max parameter can be specified to increase or decrease results.
    Examples:
     - SITE NEW
     - SITE NEW -max 30
     - SITE NEW APPS
     - SITE NEW -max 30 APPS

- SITE NEWDATE [date area]
    Description:
     - Create the dated directories and today links.
     - The "date area" parameter is optional, to use a custom newdate area rather
       the default one. See the configuration file for more details.
     - Newdate is usually run by ioFTPD's scheduler; however, this site command can
       be used to ensure the configuration is correct.
    Examples:
     - SITE NEWDATE

- SITE NUKE <directory> <multiplier> <reason>
    Description:
     - Nuke the specified directory.
     - The credit multiplier must be a number and not greater then the max multiplier.
     - The reason parameter is the reason why directory was nuked.
    Examples:
     - SITE NUKE Something.Cool-NX 5 not.so.cool

- SITE NUKES [-max <limit>] [release]
    Description:
     - Displays the latest nuked directories, along with their age and nuker.
     - The default number of results is 10, unless the -max parameter is specified.
     - The -max parameter can be specified to increase or decrease results.
    Examples:
     - SITE NUKES
     - SITE NUKES -max 15
     - SITE NUKES *something*
     - SITE NUKES -max 15 *something*

- SITE NUKETOP [-max <limit>] [group]
    Description:
     - Displays all users who receive the most nukes, or users in the specified group.
     - The default number of results is 10, unless the -max parameter is specified.
     - The -max parameter can be specified to increase or decrease results.
    Examples:
     - SITE NUKETOP
     - SITE NUKETOP -max 15
     - SITE NUKETOP iND
     - SITE NUKETOP -max 15 iND

- SITE ONEL [message]
    Description:
     - A mini-message board, the users can post or view messages.
     - If the 'message' parameter is specified, the text is added to the one-lines log.
    Examples:
     - SITE ONEL
     - SITE ONEL This is a message.

- SITE OPEN
    Description:
     - Re-open the FTP, if the site was closed.
    Examples:
     - SITE OPEN

- SITE PRE <area> <directory>
    Description:
     - Pre the specified directory to the specified area.
     - To view the latest pres, use "history" as the target area.
     - To view pre statistic, use "stats" as the target area.
    Examples:
     - SITE PRE HISTORY
     - SITE PRE STATS
     - SITE PRE APPS Something.Cool-NX
     - SITE PRE XXX ioFTPD.Gone.Wild.Vol.2.XviD-NX

- SITE REBUILD
    Description:
     - Updates the file and directory dupe database by scanning predefined paths.
    Examples:
     - SITE REBUILD

- SITE REQDEL <request id>
    Description:
     - Delete the specified request.
     - To view current requests, type 'SITE REQUEST'.
     - Only the original requester or siteops may delete requests.
    Examples:
     - SITE REQDEL 003

- SITE REQFILL <request id>
    Description:
     - Fill the specified request.
     - To view current requests, type 'SITE REQUEST'.
    Examples:
     - SITE REQFILL 005

- SITE REQUEST <request>
    Description:
     - Add the specified item to the request list.
     - If no request is specified the current requests are shown.
    Examples:
     - SITE REQUEST Something.Cool-NX

- SITE REQUESTS
    Description:
     - Displays current requests.
    Examples:
     - SITE REQUESTS

- SITE REQWIPE
    Description:
     - Automatically removes filled requests that are older then the defined limit.
     - The maximum age option is defined as 'req(MaxAge)' in the configuration.
    Examples:
     - SITE REQWIPE

- SITE RESETSTATS <-all/stats>
    Description:
     - Reset the download and upload stats of all users.
     - If the '-all' parameter is specified, all stats types are reset.
     - Several stats types can be specified at once.
     - Stats types include: alldn allup monthdn monthup wkdn wkup daydn dayup.
    Examples:
     - SITE RESETSTATS -all
     - SITE RESETSTATS alldn allup
     - SITE RESETSTATS wkdn wkup daydn dayup

- SITE RESETUSER <username>
    Description:
     - Reset the download and upload stats of the specified user.
     - If reset credit's option is enabled, the user's credits are also reset.
    Examples:
     - SITE RESETUSER someuser

- SITE ROTATELOGS
    Description:
     - Rotates the log files defined in the configuration file.
     - This event is usually run by ioFTPD's scheduler.
    Examples:
     - SITE ROTATELOGS

- SITE RULES [section]
    Description:
     - Displays the site rules, if no section is specified all rules are shown.
    Examples:
     - SITE RULES
     - SITE RULES APPS

- SITE SEARCH [-max <limit>] <release>
    Description:
     - Search the directory dupe log for a specific release.
     - Same as 'SITE DUPE' but with simpler output.
    Examples:
     - SITE SEARCH *Something*Cool*
     - SITE SEARCH -max 30 *Something*Cool*

- SITE SIZE <directory>
    Description:
     - Displays the number of files, directories, and the total size of the
       specified directory.
    Examples:
     - SITE SIZE Something.Cool-NX

- SITE TAKE <username> <credits> [credit section]
    Description:
     - Take credits from a user.
     - A size unit can be specified for the credits parameter (KB, MB or GB).
     - The credit section parameter can be specified to take credits from a
       specific section, the default section is 0.
    Examples:
     - SITE TAKE someuser 50
     - SITE TAKE someuser 50KB 3
     - SITE TAKE someuser 100GB 6

- SITE TRAFFIC [username/=group]
    Description:
     - Displays traffic statistic of a specific user, group of users, or all users.
     - If no parameter is specified, all users are shown.
    Examples:
     - SITE TRAFFIC
     - SITE TRAFFIC someuser
     - SITE TRAFFIC =thegroup

- SITE UNDUPE [-d] <filename/directory>
    Description:
     - Remove an entry from the file or directory database.
     - If the -d parameter is specified, directories are unduped rather then files.
    Examples:
     - SITE UNDUPE somecool.rar
     - SITE UNDUPE somecool.r*
     - SITE UNDUPE -d Something.Cool-NX

- SITE UNNUKE <directory> <reason>
    Description:
     - Unnuke the specified directory.
     - The reason parameter is the reason why directory was unnuked.
    Examples:
     - SITE UNNUKE Something.Cool-NX very.cool

- SITE UNNUKES [-max <limit>] [release]
    Description:
     - Display the latest unnuked directories, along with their age and unnuker.
     - The default number of results is 10, unless the -max parameter is specified.
     - The -max parameter can be specified to increase or decrease results.
    Examples:
     - SITE UNNUKES -max 15
     - SITE UNNUKES *something*
     - SITE UNNUKES -max 15 *something*

- SITE WEEKLY [<username/=group> <section>,<credits mb>]
    Description:
     - To add(+) or subtract(-) credits from a target, use the appropriate sign. If
       no sign is specified, the target's credits will be changed to the amount.
     - To remove a user or group, simply enter the target's section and credits again.
    Examples:
     - SITE WEEKLY
     - SITE WEEKLY neoxed 0,1000
     - SITE WEEKLY =STAFF 0,+5000

- SITE WEEKLYSET
    Description:
     - Assigns the credits to the users and groups.
     - Weeklyset is usually run by ioFTPD's scheduler; however, this site command
       can be used to ensure the configuration is correct.
    Examples:
     - SITE WEEKLYSET

- SITE WHO
    Description:
     - Displays information about who's online.
     - Hidden users, groups, and paths are only shown to siteops.
    Examples:
     - SITE WHO

- SITE WIPE <directory/filename>
    Description:
     - Remove a directory recursively or delete a file.
     - Wiping is preferred over deleting, since deleting removes the owner's credits.
    Examples:
     - SITE WIPE some.old.rar
     - SITE WIPE Something.Very.Old-NX


################################################################################
# 5. Text Templates                                                            #
################################################################################

    The template parser was added to a few portions of nxTools to allow users
to customize the script's output. So far the only customizable SITE commands,
and their available cookies are:

  - Viewing Approved Releases (SITE APPROVE LIST)
      New.Header       : N/A
      New.Body         : %(num) %(age) %(user) %(group) %(release)
      New.None         : N/A
      New.Footer       : N/A

  - Search Directory Database (SITE DUPE)
      Dupe.Header      : N/A
      Dupe.Body        : %(sec) %(min) %(hour) %(day) %(month) %(year2) %(year4) %(num) %(user) %(group) %(release) %(path)
      Dupe.None        : N/A
      Dupe.Footer      : %(found) %(total)

  - Search File Database (SITE FDUPE)
      FileDupe.Header  : N/A
      FileDupe.Body    : %(sec) %(min) %(hour) %(day) %(month) %(year2) %(year4) %(num) %(user) %(group) %(file)
      FileDupe.None    : N/A
      FileDupe.Footer  : %(found) %(total)

  - Viewing New Releases (SITE NEW)
      New.Header       : N/A
      New.Error        : %(sections)
      New.Body         : %(num) %(age) %(user) %(group) %(section) %(release) %(path)
      New.None         : N/A
      New.Footer       : N/A

  - Viewing One-lines (SITE ONEL)
      OneLines.Header  : N/A
      OneLines.Body    : %(sec) %(min) %(hour) %(day) %(month) %(year2) %(year4) %(user) %(group) %(message)
      OneLines.None    : N/A
      OneLines.Footer  : N/A

  - Viewing Requests (SITE REQUESTS)
      Requests.Header  : N/A
      Requests.Body    : %(age) %(id) %(user) %(group) %(request)
      Requests.None    : N/A
      Requests.Footer  : N/A

  - Viewing Rules (SITE RULES)
      Note: If the rule is longer then the defined width, the config option is
            named rules(LineWidth), they are wrapped to several lines, in which
            case all following lines use the MultiLine template.
      Rules.Header     : %(sections)
      Rules.Section    : %(section) %(sections)
      Rules.SingleLine : %(num) %(punishment) %(rule) %(section)
      Rules.MultiLine  : %(num) %(punishment) %(rule) %(section)
      Rules.Footer     : %(sections)

  - Search Directory Database (SITE SEARCH)
      Search.Header    : N/A
      Search.Body      : %(sec) %(min) %(hour) %(day) %(month) %(year2) %(year4) %(num) %(user) %(group) %(release) %(path)
      Search.None      : N/A
      Search.Footer    : %(found) %(total)

  - Viewing Who's Online (SITE WHO)
      Who.Header       : N/A
      Who.Download     : %(user) %(group) %(tagline) %(file) %(speed)
      Who.Idle         : %(user) %(group) %(tagline) %(idle)
      Who.Upload       : %(user) %(group) %(tagline) %(file) %(speed)
      Who.Footer       : N/A


################################################################################
# 6. Todo List                                                                 #
################################################################################

- Rewrite nuker to use the nukees stored in Nukes.db, fall back to read.

- Write user/credits during pre (chown + group nukes == no loss).

- Add support for text justification in message templates (left, right, and center).

- Make SITE GINFO customizable.


################################################################################
# 7. Bugs and Comments                                                         #
################################################################################

    If you have any problems with this script; or you found a bug, spelling
mistake or grammar error, please report it to me. If it's a technical issue,
be sure you can reproduce the problem, so I can find a solution.

IRC  : neoxed <#ioFTPD at EFnet>
Email: neoxed@gmail.com


################################################################################
# 8. License                                                                   #
################################################################################

    See the "license.txt" file for details.
