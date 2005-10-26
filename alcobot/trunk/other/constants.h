/*++

AlcoBot - Alcoholicz site bot.
Copyright (c) 2005 Alcoholicz Scripting Team

Module Name:
    constants.h

Author:
    neoxed (neoxed@gmail.com) Oct 24, 2005

Abstract:
    Defines variable constants and log formats for Project-ZS-NG v1.1
    (trunk). You must replace Project-ZS-NG's constants.h with this one
    and recompile it.

--*/

#ifndef __CONSTANTS_H_
#define __CONSTANTS_H_

#include "zsconfig.h"
#include "zsconfig.defaults.h"

#define F_IGNORED                   254
#define F_BAD                       255
#define F_NFO                       253
#define F_DELETED                   0
#define F_CHECKED                   1
#define F_NOTCHECKED                2

#define TRUE                        1
#define FALSE                       0

#define DISABLED                    NULL

#define FILE_MAX                    256
#define MAXIMUM_FILES_IN_RELEASE    1024

#define PROGTYPE_ZIPSCRIPT          2
#define PROGTYPE_POSTDEL            4
#define PROGTYPE_CLEANUP            8
#define PROGTYPE_DATACLEANER        16
#define PROGTYPE_RESCAN             32

#define audio_announce_norace_complete_type             "COMPLETE"
#define audio_cbr_announce_norace_complete_type         "COMPLETE"
#define audio_vbr_announce_norace_complete_type         "COMPLETE"
#define general_announce_norace_complete_type           "COMPLETE"
#define other_announce_norace_complete_type             "COMPLETE"
#define rar_announce_norace_complete_type               "COMPLETE"
#define video_announce_norace_complete_type             "COMPLETE"
#define zip_announce_norace_complete_type               "COMPLETE"

#define audio_announce_race_complete_type               "COMPLETE_RACE"
#define audio_cbr_announce_race_complete_type           "COMPLETE_RACE"
#define audio_vbr_announce_race_complete_type           "COMPLETE_RACE"
#define general_announce_race_complete_type             "COMPLETE_RACE"
#define other_announce_race_complete_type               "COMPLETE_RACE"
#define rar_announce_race_complete_type                 "COMPLETE_RACE"
#define video_announce_race_complete_type               "COMPLETE_RACE"
#define zip_announce_race_complete_type                 "COMPLETE_RACE"

#define audio_announce_cbr_update_type                  "UPDATE_MP3"
#define audio_announce_vbr_update_type                  "UPDATE_MP3"
#define general_announce_update_type                    "UPDATE"
#define other_announce_update_type                      "UPDATE"
#define rar_announce_update_type                        "UPDATE"
#define video_announce_update_type                      "UPDATE"
#define zip_announce_update_type                        "UPDATE"

#define audio_announce_race_type                        "RACE"
#define general_announce_race_type                      "RACE"
#define other_announce_race_type                        "RACE"
#define rar_announce_race_type                          "RACE"
#define video_announce_race_type                        "RACE"
#define zip_announce_race_type                          "RACE"

#define audio_announce_newleader_type                   "NEWLEADER"
#define general_announce_newleader_type                 "NEWLEADER"
#define other_announce_newleader_type                   "NEWLEADER"
#define rar_announce_newleader_type                     "NEWLEADER"
#define video_announce_newleader_type                   "NEWLEADER"
#define zip_announce_newleader_type                     "NEWLEADER"

#define audio_announce_norace_halfway_type              "HALFWAY"
#define general_announce_norace_halfway_type            "HALFWAY"
#define other_announce_norace_halfway_type              "HALFWAY"
#define rar_announce_norace_halfway_type                "HALFWAY"
#define video_announce_norace_halfway_type              "HALFWAY"
#define zip_announce_norace_halfway_type                "HALFWAY"

#define audio_announce_race_halfway_type                "HALFWAY_RACE"
#define general_announce_race_halfway_type              "HALFWAY_RACE"
#define other_announce_race_halfway_type                "HALFWAY_RACE"
#define rar_announce_race_halfway_type                  "HALFWAY_RACE"
#define video_announce_race_halfway_type                "HALFWAY_RACE"
#define zip_announce_race_halfway_type                  "HALFWAY_RACE"

#define audio_announce_sfv_type                         "SFV"
#define general_announce_sfv_type                       "SFV"
#define other_announce_sfv_type                         "SFV"
#define rar_announce_sfv_type                           "SFV"
#define video_announce_sfv_type                         "SFV"

#define general_doublesfv_type                          "DOUBLE_SFV"
#define general_incomplete_type                         "INCOMPLETE"
#define speed_type                                      "SPEEDTEST"

#define general_badbitrate_type                         "WARN_BITRATE"
#define general_badgenre_type                           "WARN_GENRE"
#define general_badpreset_type                          "WARN_PRESET"
#define general_badyear_type                            "WARN_YEAR"

#define bad_file_0size_type                             "BAD_FILE_0SIZE"
#define bad_file_bitrate_type                           "BAD_FILE_BITRATE"
#define bad_file_crc_type                               "BAD_FILE_CRC"
#define bad_file_disallowed_type                        "BAD_FILE_TYPE"
#define bad_file_genre_type                             "BAD_FILE_GENRE"
#define bad_file_nfo_type                               "BAD_FILE_DUPENFO"
#define bad_file_nfodenied_type                         "BAD_FILE_NFODENIED"
#define bad_file_nosfv_type                             "BAD_FILE_NOSFV"
#define bad_file_password_type                          "BAD_FILE_PASSWORD"
#define bad_file_sfv_type                               "BAD_FILE_SFV"
#define bad_file_vbr_preset_type                        "BAD_FILE_PRESET"
#define bad_file_wrongdir_type                          "BAD_FILE_WRONGDIR"
#define bad_file_year_type                              "BAD_FILE_YEAR"
#define bad_file_zip_type                               "BAD_FILE_ZIP"

/* Output of racestats binary */
#define stats_line          "\"%F\" \"%f\" \"%u\" \"%g\" %C0 %c0"

/* Some crap */
#define post_stats          NULL
#define winner              "winner"
#define loser               "loser"

/* Put in %C cookie */
#define user_info           "\"%u\" \"%g\" \"%.3m\" \"%.3S\" \"%.3p\" \"%f\""

/* Put in %c cookie */
#define group_info          "\"%g\" \"%.3m\" \"%.3S\" \"%.3p\" \"%f\""

/* Put in %l cookie */
#define fastestfile         "\"%u\" \"%g\" \"%.3F\""

/* Put in %L cookie */
#define slowestfile         "\"%u\" \"%g\" \"%.3S\""

/* Put in %j cookie */
#define audio_vbr           "\"%y\" \"%W\" \"%x\" \"%w\" \"%Y\" \"%X\" \"%h\" \"%Q\" \"VBR\""
#define audio_cbr           "\"%y\" \"%W\" \"%x\" \"%w\" \"%Y\" \"%X\" \"%h\" \"%Q\" \"CBR\""

/* Put in %R cookie */
#define racersplit          ""
#define racersplit_prior    ""
#define racersmsg           "%u %g"

/* Put in %t cookie */
#define user_top            "%n %u %g %.3m %.3S %.3p %f %D %W %M %A"

/* Put in %T cookie */
#define group_top           "%n %g %.3m %.3S %.3p %f"

/* Complete cookies (race) */
#define audio_complete              "\"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\" \"%u\" \"%g\" %l %L %C0 %c0 \"%t\" \"%T\""
#define other_complete              "\"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\" \"%u\" \"%g\" %l %L %C0 %c0 \"%t\" \"%T\""
#define rar_complete                "\"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\" \"%u\" \"%g\" %l %L %C0 %c0 \"%t\" \"%T\""
#define video_complete              "\"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\" \"%u\" \"%g\" %l %L %C0 %c0 \"%t\" \"%T\""
#define zip_complete                "\"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\" \"%u\" \"%g\" %l %L %C0 %c0 \"%t\" \"%T\""

/* Complete cookies (norace) */
#define audio_norace_complete       "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\""
#define other_norace_complete       "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\""
#define rar_norace_complete         "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\""
#define video_norace_complete       "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\""
#define zip_norace_complete         "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%f\" \"%d\""

/* Halfway cookies (race) */
#define audio_halfway               "\"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" %C0 %c0"
#define other_halfway               "\"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" %C0 %c0"
#define rar_halfway                 "\"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" %C0 %c0"
#define video_halfway               "\"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" %C0 %c0"
#define zip_halfway                 "\"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" %C0 %c0"

/* Halfway cookies (norace) */
#define audio_norace_halfway        "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""
#define other_norace_halfway        "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""
#define rar_norace_halfway          "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""
#define video_norace_halfway        "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""
#define zip_norace_halfway          "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3A\" \"%.3a\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""

/* Update cookies */
#define audio_update                "\"%U\" \"%G\" \"%K\" \"%.3e\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" %j"
#define other_update                "\"%U\" \"%G\" \"%K\" \"%.3e\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""
#define rar_update                  "\"%U\" \"%G\" \"%K\" \"%.3e\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""
#define video_update                "\"%U\" \"%G\" \"%K\" \"%.3e\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""
#define zip_update                  "\"%U\" \"%G\" \"%K\" \"%.3e\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\""

/* New leader cookies */
#define audio_newleader             "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\" %C0 %c0"
#define other_newleader             "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\" %C0 %c0"
#define rar_newleader               "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\" %C0 %c0"
#define video_newleader             "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\" %C0 %c0"
#define zip_newleader               "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\" %C0 %c0"

/* Race cookies */
#define audio_race                  "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\""
#define other_race                  "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\""
#define rar_race                    "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\""
#define video_race                  "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\""
#define zip_race                    "\"%U\" \"%G\" \"%K\" \"%.3m\" \"%.3S\" \"%.3p\" \"%F\" \"%M\" \"%f\" \"%d\" \"%$\" \"%u\" \"%g\" \"%R\""

/* SFV cookies */
#define audio_sfv                   "\"%U\" \"%G\" \"%K\" \"%.3p\" \"%F\" \"%M\" \"%f\""
#define other_sfv                   "\"%U\" \"%G\" \"%K\" \"%.3p\" \"%F\" \"%M\" \"%f\""
#define rar_sfv                     "\"%U\" \"%G\" \"%K\" \"%.3p\" \"%F\" \"%M\" \"%f\""
#define video_sfv                   "\"%U\" \"%G\" \"%K\" \"%.3p\" \"%F\" \"%M\" \"%f\""

/* Audio warnings */
#define audio_cbr_warn_msg          "\"%U\" \"%G\" \"%K\" \"%n\" \"%X\""
#define audio_genre_warn_msg        "\"%U\" \"%G\" \"%K\" \"%n\" \"%w\""
#define audio_vbr_preset_warn_msg   "\"%U\" \"%G\" \"%K\" \"%n\" \"%I\""
#define audio_year_warn_msg         "\"%U\" \"%G\" \"%K\" \"%n\" \"%Y\""

/* Bad files */
#define bad_file_msg                "\"%U\" \"%G\" \"%K\" \"%n\""

/* Double SFV */
#define deny_double_msg             "\"%U\" \"%G\" \"%K\" \"%n\""

/* Incomplete release */
#define incompletemsg               "\"%U\" \"%G\" \"%K\""

/* Speed test */
#define speed_announce              "\"%U\" \"%G\" \"%K\" \"%.3/\" \"%.3S\""

enum ReleaseTypes {
    RTYPE_NULL = 0,
    RTYPE_RAR  = 1,
    RTYPE_OTHER = 2,
    RTYPE_AUDIO = 3,
    RTYPE_VIDEO = 4,
    RTYPE_INVALID,
};

#endif
