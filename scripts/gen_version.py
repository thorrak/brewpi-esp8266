# Generate the Version.h file
# Stitches together various bits of version information from the environment

import subprocess
import os

# This was the value used in the static Version.h.  It is unclear what merits
# an increment, so it is being included unchanged.
release = "0.2.4"

# Get the name of the "nearest" tag (decorated with revision information if
# changes have happened since that tag)
# See the man page of git-describe for more details
tag_name = (
    subprocess.check_output(["git", "describe", "--always"])
    .strip()
    .decode("utf-8")
)

# git revision
git_rev = (
    subprocess.check_output(["git", "rev-parse", "--short", "HEAD"])
    .strip()
    .decode("utf-8")
)



template = f"""
#pragma once
/******************************************************************************
*** ****  WARNING **** ****

This file is auto-generated.  Any changes made here will be destroyed during
the next build.  To make persistent changes, edit the template in
/scripts/gen_version.py
******************************************************************************/

namespace Config {{
    namespace Version {{
        constexpr auto release = "{release}";
        constexpr auto git_tag = "{tag_name}";
        constexpr auto git_rev = "{git_rev}";
    }}
}};
"""

with open('src/Version.h', 'w') as f:
    f.write(template)
