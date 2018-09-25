#pragma once
#include <glm/glm.hpp>
#include <vector>

//struct that stores all the game state

static constexpr const uint32_t MAX_SNAKE_SIZE = 300;
static constexpr const uint32_t MAX_NUM_APPLES = 5;
static constexpr const uint32_t PAIR_SIZE = 2;
static constexpr const glm::vec2 UP = glm::vec2(0.0f, 1.0f);
static constexpr const glm::vec2 DOWN = glm::vec2(0.0f, -1.0f);
static constexpr const glm::vec2 LEFT = glm::vec2(-1.0f, 0.0f);
static constexpr const glm::vec2 RIGHT = glm::vec2(1.0f, 0.0f);

struct Game {
	std::vector<glm::vec2> snake1 = {glm::vec2(1.0f, 1.0f)}; //snake1 start position
	std::vector<glm::vec2> snake2 = {glm::vec2(19.0f, 15.0f)}; //snake2 start position

	glm::vec2 snake1_direction = UP;
	glm::vec2 snake2_direction = DOWN;

	std::vector<glm::vec2> apples; //apple locations, needs to be randomly generated
																 //should always be size 5

	bool gameOver = false; //is the game over?
	bool playerOne = false; //if so, is the winner player 1?
	bool gameStarted = false;
	uint32_t id = -1;

	void update(float time);

	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float weight = 20.0f;
	static constexpr const float height = 16.0f;
	static constexpr const float unit = 0.5f;
};

//struct for serialization, only has snake data
// client --> server --> client
struct Snake {
	float positions [MAX_SNAKE_SIZE][PAIR_SIZE];
	float direction [PAIR_SIZE];
};

//serialization structs for apple locs and game status
// server --> client
struct ApplesAndStatus {
	float positions [MAX_NUM_APPLES][PAIR_SIZE];
	bool gameOver;
	bool playerOne;
	bool gameStarted;
};

//format to send data in
struct SerializedState {
	Snake player1;
	Snake player2;
	ApplesAndStatus status;
	uint32_t id;
};

//format server stores data in
struct ServerState {
	std::vector<glm::vec2> snake1 = {glm::vec2(1.0f, 1.0f)}; //snake1 start position
	std::vector<glm::vec2> snake2 = {glm::vec2(19.0f, 15.0f)}; //snake2 start position

	glm::vec2 snake1_direction = UP;
	glm::vec2 snake2_direction = DOWN;
	std::vector<glm::vec2> apples = std::vector<glm::vec2>(MAX_NUM_APPLES);
																//apple locations, needs to be randomly generated
																 //should always be size 5

	bool gameOver = false; //is the game over?
	bool playerOne = false; //if so, is the winner player 1?
	bool gameStarted = false; //has the game started??

	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float width = 20.0f;
	static constexpr const float height = 16.0f;
	static constexpr const float unit = 0.5f;
};

//isClient is true when a client is calling the function
void state_to_serialized(ServerState* data, SerializedState* serialized, uint32_t id, bool isClient);
uint32_t serialized_to_state(SerializedState* serialized, ServerState* data, bool isClient, uint32_t conn_id);


/**
x-y coords:
x: 0-20
y: 0-16

valid bounds:
x: 1-19
y: 1-15

start1: 1,1
start2: 19,15

index --> real render location divide by 2
**/
