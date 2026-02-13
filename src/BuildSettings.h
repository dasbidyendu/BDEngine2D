#pragma once

#define BD_SHIPPING 

#ifdef BD_SHIPPING
#define BD_EDITOR_ENABLED 0
#define BD_LOG_LEVEL LOG_ERROR
#define BD_START_SCENE "assets/scenes/main.scene"
#else
#define BD_EDITOR_ENABLED 1
#define BD_LOG_LEVEL LOG_ALL
#endif