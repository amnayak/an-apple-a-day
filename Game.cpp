#include "Game.hpp"
#include <algorithm>
#include <iostream>


//update game state at every interval
//called by the client afaik
void Game::update(float time) {
	//ball += ball_velocity * time;
	//TODO fiddle with the "time" param
	(void)time;
	if (!gameStarted) {
		return;
	}
	//insert a new position into the snake vector
	//if it didn't collide with an apple, remove last element
	if (id % 2 == 0) { //if i'm player 1
		snake1.insert(snake1.begin(), snake1[0] + snake1_direction);
		if ( std::find(apples.begin(), apples.end(), snake1[0]) != apples.end() ) {
			snake1.pop_back();
		}
	} else { //if i'm player 2
		snake2.insert(snake2.begin(), snake2[0] + snake2_direction);
		if ( std::find(apples.begin(), apples.end(), snake2[0]) != apples.end() ) {
			snake2.pop_back();
		}
	}
}

void state_to_serialized(ServerState* data, SerializedState* serialized, uint32_t id, bool isClient) {

	uint32_t i = 0;
	for (glm::vec2 vec: data->snake1) {
		serialized->player1.positions[i][0] = vec.x;
		serialized->player1.positions[i][1] = vec.y;
	}
	//TODO: potential bug: how do you know what region of the serialized
				//array is valid data? rn it's 0-terminated

	i=0;
	for (glm::vec2 vec: data->snake2) {
		serialized->player2.positions[i][0] = vec.x;
		serialized->player2.positions[i][1] = vec.y;
	}

	serialized->player1.direction[0] = data->snake1_direction.x;
	serialized->player1.direction[1] = data->snake1_direction.y;

	serialized->player2.direction[0] = data->snake2_direction.x;
	serialized->player2.direction[1] = data->snake2_direction.y;

	i=0;
	for(glm::vec2 vec: data->apples) {
		serialized->status.positions[i][0] = vec.x;
		serialized->status.positions[i][1] = vec.y;
	}

	assert(i <= 4);

	serialized->status.gameOver = data->gameOver;
	serialized->status.playerOne = data->playerOne;
	serialized->status.gameStarted = data->gameStarted;
	serialized->id = id;

}

uint32_t serialized_to_state(SerializedState* serialized, ServerState* data, bool isClient, uint32_t conn_id) {
	/**
	Client holds authority of: it's own snake's position & data
	Server holds authority of: win conditions, apple locations,
	**/

	(void) conn_id;
	std::cout << "serial to state 1" << std::endl;
	for (uint32_t i = 0; i < MAX_SNAKE_SIZE; i++) {
		if (serialized->player1.positions[i][0] == 0 && //if you reach
				serialized->player1.positions[i][1] == 0) { // the end of valid data
				i = MAX_SNAKE_SIZE;
		} else {
			data->snake1[i][0] = serialized->player1.positions[i][0];
			data->snake1[i][1] = serialized->player1.positions[i][1];
		}
	}
std::cout << "serial to state 2" << std::endl;
	for (uint32_t i = 0; i < MAX_SNAKE_SIZE; i++) {
		if (serialized->player2.positions[i][0] == 0 && //if you reach
				serialized->player2.positions[i][1] == 0) { // the end of valid data
				i = MAX_SNAKE_SIZE;
		} else {
		data->snake2[i][0] = serialized->player2.positions[i][0];
		data->snake2[i][1] = serialized->player2.positions[i][1];
		}
	}
std::cout << "serial to state 3" << std::endl;
	data->snake1_direction.x = serialized->player1.direction[0];
	data->snake1_direction.y = serialized->player1.direction[1];

	data->snake2_direction.x = serialized->player2.direction[0];
	data->snake2_direction.y = serialized->player2.direction[1];
std::cout << "serial to state 4" << std::endl;
	if (isClient){
		for (uint32_t i = 0; i<MAX_NUM_APPLES; i++) {
			std::cout << "serial to state 4.1 " << i << std::endl;
			std::cout << "from loop: " <<serialized << std::endl;
			if (serialized->status.positions[i][0] == 0 && //if you reach
					serialized->status.positions[i][1] == 0) { // the end of valid data

					std::cout << "serial to state 4.2" << std::endl;
					i = MAX_NUM_APPLES;
					std::cout << "serial to state 4.5" << std::endl;
			} else {
				std::cout << "deref data->apples i" << data->apples.size() << std::endl;
				data->apples[i][0] = serialized->status.positions[i][0];
				data->apples[i][1] = serialized->status.positions[i][1];
				std::cout << "deref data worked" << std::endl;
			}
		}
std::cout << "serial to state 4.8" << std::endl;
		data->gameOver = serialized->status.gameOver;
		data->playerOne = serialized->status.playerOne;
		data->gameStarted = serialized->status.gameStarted;
	}
std::cout << "serial to state 5" << std::endl;
	return serialized->id;

}
