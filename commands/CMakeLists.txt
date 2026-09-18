# Command system: camera, console, preset, setup, scene
target_sources(ptz_commander PRIVATE
    Arguments.h
    ResolveCameras.h
    Command.h
    CommandHandler.cpp
    CommandHandler.h

    camera/AliasCommand.cpp
    camera/AliasCommand.h
    camera/ListCommand.cpp
    camera/ListCommand.h
    camera/MoveCommand.cpp
    camera/MoveCommand.h
    camera/PushCommand.cpp
    camera/PushCommand.h
    camera/ShowCommand.cpp
    camera/ShowCommand.h

    console/ClearCommand.cpp
    console/ClearCommand.h

    preset/ApplyCommand.cpp
    preset/ApplyCommand.h
    preset/DiscardCommand.cpp
    preset/DiscardCommand.h
    preset/ListCommand.cpp
    preset/ListCommand.h
    preset/StoreCommand.cpp
    preset/StoreCommand.h

    setup/ListCommand.cpp
    setup/ListCommand.h
    setup/LoadCommand.cpp
    setup/LoadCommand.h
    setup/SaveCommand.cpp
    setup/SaveCommand.h

    scene/AddCommand.cpp
    scene/AddCommand.h
    scene/ApplyCommand.cpp
    scene/ApplyCommand.h
    scene/DiscardCommand.cpp
    scene/DiscardCommand.h
    scene/ListCommand.cpp
    scene/ListCommand.h
    scene/RemoveCommand.cpp
    scene/RemoveCommand.h
    scene/ShowCommand.cpp
    scene/ShowCommand.h
    scene/TourCommand.cpp
    scene/TourCommand.h
)
