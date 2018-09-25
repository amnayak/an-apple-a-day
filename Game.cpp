#include "Game.hpp"
#include <algorithm>
#include <iostream>


//update game state at every interval
void Game::update(float time) {
}

void state_to_serialized(ServerState* data, SerializedState* serialized, uint32_t id, bool isClient) {
	uint32_t i = 0;

	for (glm::vec2 vec: data->snake1) {
		serialized->player1.positions[i][0] = vec.x;
		serialized->player1.positions[i][1] = vec.y;
		i++;
	}
		// zero-terminated
	serialized->player1.positions[i][0] = 0;
	serialized->player1.positions[i][1] = 0;

	i=0;
	for (glm::vec2 vec: data->snake2) {
		serialized->player2.positions[i][0] = vec.x;
		serialized->player2.positions[i][1] = vec.y;
		i++;
	}
	// zero-terminated
	serialized->player2.positions[i][0] = 0;
	serialized->player2.positions[i][1] = 0;

	serialized->player1.direction[0] = data->snake1_direction.x;
	serialized->player1.direction[1] = data->snake1_direction.y;

	serialized->player2.direction[0] = data->snake2_direction.x;
	serialized->player2.direction[1] = data->snake2_direction.y;

	i=0;
	for(glm::vec2 vec: data->apples) {
		serialized->status.positions[i][0] = vec.x;
		serialized->status.positions[i][1] = vec.y;
		i++;
	}
	// zero-terminated
	serialized->status.positions[i][0] = 0;
	serialized->status.positions[i][1] = 0;

	assert(i <= 5);
	serialized->status.gameOver = data->gameOver;
	serialized->status.playerOne = data->playerOne;
	serialized->status.gameStarted = data->gameStarted;
	serialized->id = id;

}

uint32_t serialized_to_state(SerializedState* serialized, ServerState* data, bool isClient, uint32_t conn_id) {
	/**
	Client holds authority of: it's own snake's direction
	Server holds authority of: win conditions, apple locations, snake positions
	**/

	(void) conn_id;
	if (isClient){
		assert(data->snake1.size() > 0);
		assert(data->snake2.size() > 0);
		data->snake1.clear();
		data->snake2.clear();

	for (uint32_t i = 0; i < MAX_SNAKE_SIZE; i++) {
		if (serialized->player1.positions[i][0] == 0 && //if you reach
				serialized->player1.positions[i][1] == 0) { // the end of valid data
				i = MAX_SNAKE_SIZE;
		} else {
			glm::vec2 elem;
			elem.x = serialized->player1.positions[i][0];
			elem.y = serialized->player1.positions[i][1];
			data->snake1.push_back(elem);
		}
	}
	for (uint32_t i = 0; i < MAX_SNAKE_SIZE; i++) {
		if (serialized->player2.positions[i][0] == 0 && //if you reach
				serialized->player2.positions[i][1] == 0) { // the end of valid data
				i = MAX_SNAKE_SIZE;
		} else {
			glm::vec2 elem;
			elem.x = serialized->player2.positions[i][0];
			elem.y = serialized->player2.positions[i][1];
			data->snake2.push_back(elem);
		}
	}
	assert(data->snake1.size() > 0);
	assert(data->snake2.size() > 0);
}
	data->snake1_direction.x = serialized->player1.direction[0];
	data->snake1_direction.y = serialized->player1.direction[1];

	data->snake2_direction.x = serialized->player2.direction[0];
	data->snake2_direction.y = serialized->player2.direction[1];

	if (isClient){
		data->apples.clear();
		for (uint32_t i = 0; i<MAX_NUM_APPLES; i++) {
			if (serialized->status.positions[i][0] == 0 && //if you reach
					serialized->status.positions[i][1] == 0) { // the end of valid data

					break;
			} else {
				glm::vec2 elem;
				elem.x = serialized->status.positions[i][0];
				elem.y = serialized->status.positions[i][1];
				data->apples.push_back(elem);

			}
		}
		data->gameOver = serialized->status.gameOver;
		data->playerOne = serialized->status.playerOne;
		data->gameStarted = serialized->status.gameStarted;
	}
	return serialized->id;

}
