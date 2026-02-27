qt_add_library(HanwhaAssets STATIC)

# Set explicit resource aliases for each file (no PREFIX needed)
set_source_files_properties(Core/Icons/Alarm.svg PROPERTIES QT_RESOURCE_ALIAS /Core/Icons/Alarm.svg)
set_source_files_properties(Core/Icons/Exit.svg PROPERTIES QT_RESOURCE_ALIAS /Core/Icons/Exit.svg)
set_source_files_properties(Core/Icons/Help.svg PROPERTIES QT_RESOURCE_ALIAS /Core/Icons/Help.svg)
set_source_files_properties(Core/Icons/Maximize.svg PROPERTIES QT_RESOURCE_ALIAS /Core/Icons/Maximize.svg)
set_source_files_properties(Core/Icons/Minimize.svg PROPERTIES QT_RESOURCE_ALIAS /Core/Icons/Minimize.svg)
set_source_files_properties(Core/Icons/Option.svg PROPERTIES QT_RESOURCE_ALIAS /Core/Icons/Option.svg)
set_source_files_properties(Core/Logos/OnlyLogo.svg PROPERTIES QT_RESOURCE_ALIAS /Core/Logos/OnlyLogo.svg)
set_source_files_properties(Core/Fonts/01HanwhaB.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/01HanwhaB.ttf)
set_source_files_properties(Core/Fonts/02HanwhaR.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/02HanwhaR.ttf)
set_source_files_properties(Core/Fonts/03HanwhaL.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/03HanwhaL.ttf)
set_source_files_properties(Core/Fonts/04HanwhaGothicB.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/04HanwhaGothicB.ttf)
set_source_files_properties(Core/Fonts/05HanwhaGothicR.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/05HanwhaGothicR.ttf)
set_source_files_properties(Core/Fonts/06HanwhaGothicL.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/06HanwhaGothicL.ttf)
set_source_files_properties(Core/Fonts/07HanwhaGothicEL.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/07HanwhaGothicEL.ttf)
set_source_files_properties(Core/Fonts/08HanwhaGothicT.ttf PROPERTIES QT_RESOURCE_ALIAS /Core/Fonts/08HanwhaGothicT.ttf)

qt_add_resources(HanwhaAssets "assets"
    FILES
        Core/Icons/Alarm.svg
        Core/Icons/Exit.svg
        Core/Icons/Help.svg
        Core/Icons/Maximize.svg
        Core/Icons/Minimize.svg
        Core/Icons/Option.svg
        Core/Logos/OnlyLogo.svg
        Core/Fonts/01HanwhaB.ttf
        Core/Fonts/02HanwhaR.ttf
        Core/Fonts/03HanwhaL.ttf
        Core/Fonts/04HanwhaGothicB.ttf
        Core/Fonts/05HanwhaGothicR.ttf
        Core/Fonts/06HanwhaGothicL.ttf
        Core/Fonts/07HanwhaGothicEL.ttf
        Core/Fonts/08HanwhaGothicT.ttf
)

target_link_libraries(HanwhaAssets
    PUBLIC
        Qt6::Quick
        Qt6::Gui
        Qt6::Qml
        Qt6::Svg
)