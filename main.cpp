#include "src/Core/Engine.h"

int main() {
	Engine game(800, 600, "BDEngine");

	game.InitGame();
	game.Run();

	return 0;
}