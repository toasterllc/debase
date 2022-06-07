constexpr const char*const UsageText = R"#(
Usage:

debase [<rev>...]
    Open the specified git revisions in debase

    Supports standard git revision syntax, such as:
        branch^ tag~2 HEAD^^^

    When no revisions are supplied, debase opens the 5 most recently
    checked-out revisions, as can be seen with:

        git reflog | grep checkout:

debase -h
debase --help
    Print this help message

debase --theme <auto|dark|light>
    Set theme
    
    With `auto`, debase will automatically determine whether the
    terminal background is dark or light and choose the
    appropriate theme.

debase --libs
    Print library acknowledgments and legalese

)#";
