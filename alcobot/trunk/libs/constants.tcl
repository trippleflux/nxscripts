#
# AlcoBot - Alcoholicz site bot.
# Copyright (c) 2005 Alcoholicz Scripting Team
#
# Module Name:
#   Constants
#
# Author:
#   neoxed (neoxed@gmail.com) June 3, 2005
#
# Abstract:
#   Variable constants (options that would not be modified in most cases).
#

namespace eval ::alcoholicz {
    # Divisor to move from one size unit to the next (usually 1000 or 1024).
    variable sizeDivisor    1024

    # Divisor to move from one speed unit to the next (usually 1000 or 1024).
    variable speedDivisor   1024

    # Subdirectories used for path parsing (case-insensitive).
    variable subDirList     {cd[0-9] dis[ck][0-9] dvd[0-9] codec codecs cover covers extra extras sample sub subs vobsub vobsubs}
}
