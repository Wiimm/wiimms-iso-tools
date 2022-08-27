
[PATHS]
# This section defines different paths. Each path may contain any number of
# references to other paths by »$(NAME)«. All names are case sensitive.
# >>> Please use '/' and not '\' as directory separator, even for Cygwin. <<<

# The base is only a helper path to allow relative paths. It is not used
# internally. For Cygwin, it is usually Windows »PROGRAMFILES«. For other
# systems it is »/usr/local«.
base	= %s

# The directory, where the tools will be installed. After installation
# it is used for updates. It is usually »$(base)/Wiimm/SZS« for Cygwin
# and »$(base)/bin« for other systems.
install	= %s

# File or directory, where a configuration should be stored on installation.
# The filename »wiimms-szs-tools.conf« is appended for directories.
# An existing file is scanned and rewritten. If empty, don't create it.
config	= %s

# The following is a directory list in the order in which the configuration
# file is searched. Uncomment a line to override the previous setting.
#   config = "$(xdg_home)/szs/"	# XDG directory of the user
#   config = "$(home)/.szs/"	# Home directory of the user
#   config = "$(xdg_etc)/"	# Standard XDG config-dir for unix based systems
#   config = "/etc/"		# Standard config-dir for unix based systems
#   config = "$(share)/"	# Directory of shared files (bad choice)
#   config = "$(install)/"	# Directory of installation

# The directory for shared files. It is usually »$(install)« for Cygwin and
# »$(base)/share/szs« for other systems.
share	= %s

# The directory in which the »titles« files are saved and searched for first.
# It is usually »$(share)« for all systems.
titles	= %s

