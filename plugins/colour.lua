colour = {
    iscommand=false
}

colour.ANSI_ESC_START = string.char(0x1b);
colour.DEFAULT = colour.ANSI_ESC_START .. "[0m";
colour.BOLD = colour.ANSI_ESC_START .. "[1m"
colour.UNDERLINE = colour.ANSI_ESC_START .. "[4m"
colour.NO_UNDERLINE = colour.ANSI_ESC_START .. "[24m"
colour.NEGATIVE = colour.ANSI_ESC_START .. "[7m"
colour.POSITIVE = colour.ANSI_ESC_START .. "[27m"
colour.BLACK = colour.ANSI_ESC_START .. "[30m"
colour.RED = colour.ANSI_ESC_START .. "[31m"
colour.GREEN = colour.ANSI_ESC_START .. "[32m"
colour.YELLOW = colour.ANSI_ESC_START .. "[33m"
colour.BLUE = colour.ANSI_ESC_START .. "[34m"
colour.MAGENTA = colour.ANSI_ESC_START .. "[35m"
colour.CYAN = colour.ANSI_ESC_START .. "[36m"
colour.LIGHTGRAY = colour.ANSI_ESC_START .. "[37m"
colour.DARKGRAY = colour.ANSI_ESC_START .. "[90m"
colour.LIGHTRED = colour.ANSI_ESC_START .. "[91m"
colour.LIGHTGREEN = colour.ANSI_ESC_START .. "[92m"
colour.LIGHTYELLOW = colour.ANSI_ESC_START .. "[93m"
colour.LIGHTBLUE = colour.ANSI_ESC_START .. "[94m"
colour.LIGHTMAGENTA = colour.ANSI_ESC_START .. "[95m"
colour.LIGHTCYAN = colour.ANSI_ESC_START .. "[96m"
colour.WHITE = colour.ANSI_ESC_START .. "[97m"

