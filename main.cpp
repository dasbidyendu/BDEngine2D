#include "src/Core/Engine.h"

int main() {
	Engine game(GetScreenWidth()/2, GetScreenHeight()/2, "BDEngine");

	game.InitGame();
	game.Run();

	return 0;
}