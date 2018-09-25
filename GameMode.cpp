#include "GameMode.hpp"
//TODO
#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>
#include <string>


Load< MeshBuffer > meshes(LoadTagDefault, [](){
	return new MeshBuffer(data_path("paddle-ball.pnc"));
});

Load< GLuint > meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});


Scene::Transform *apple_transforms [MAX_NUM_APPLES];
Scene::Transform *snake1_transforms [MAX_SNAKE_SIZE];
Scene::Transform *snake2_transforms [MAX_SNAKE_SIZE];

Scene::Camera *camera = nullptr;

Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//load transform hierarchy:
	ret->load(data_path("paddle-ball.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
		Scene::Object *obj = s.new_object(t);

		obj->program = vertex_color_program->program;
		obj->program_mvp_mat4  = vertex_color_program->object_to_clip_mat4;
		obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
		obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

		MeshBuffer::Mesh const &mesh = meshes->lookup(m);
		obj->vao = *meshes_for_vertex_color_program;
		obj->start = mesh.start;
		obj->count = mesh.count;
	});

	//look up paddle and ball transforms:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		for(uint32_t i = 0; i<MAX_NUM_APPLES; i++) {
			std::string name = "Apple" + std::to_string(i + 1);
			if (t->name == name) {
				apple_transforms[i] = t;
			}
		}
		for(uint32_t i = 0; i<MAX_SNAKE_SIZE; i++) {
			std::string name = "Ball1.00" + std::to_string(i);
			if (t->name == name) {
				snake1_transforms[i] = t;
			}
		}
		for(uint32_t i = 0; i<MAX_SNAKE_SIZE; i++) {
			std::string name = "Ball2.00" + std::to_string(i);
			if (t->name == name) {
				snake2_transforms[i] = t;
			}
		}
	}

	//look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
	return ret;
});

GameMode::GameMode(Client &client_) : client(client_) {
	client.connection.send_raw("h", 1); //send a 'hello' to the server
	state.snake1.push_back(glm::vec2(1.0f, 1.0f));
	state.snake2.push_back(glm::vec2(19.0f, 15.0f));
}

GameMode::~GameMode() {
}

bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.scancode == SDL_SCANCODE_W || evt.key.keysym.scancode == SDL_SCANCODE_UP) {
				if (state.id % 2 == 0 && state.snake1_direction != DOWN) { //if you're player 1
					state.snake1_direction = UP;
				} else if (state.id % 2 == 1 && state.snake1_direction != DOWN) {
					state.snake2_direction = UP;
				}
				return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_S || evt.key.keysym.scancode == SDL_SCANCODE_DOWN) {
				if (state.id % 2 == 0 && state.snake1_direction != UP) { //if you're player 1
					state.snake1_direction = DOWN;
				} else if (state.id % 2 == 1 && state.snake1_direction != UP) {
					state.snake2_direction = DOWN;
				}
				return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_A || evt.key.keysym.scancode == SDL_SCANCODE_LEFT) {
				if (state.id % 2 == 0 && state.snake1_direction != RIGHT) { //if you're player 1
					state.snake1_direction = LEFT;
				} else if (state.id % 2 == 1 && state.snake1_direction != RIGHT) {
					state.snake2_direction = LEFT;
				}
				return true;
		} else if (evt.key.keysym.scancode == SDL_SCANCODE_D || evt.key.keysym.scancode == SDL_SCANCODE_RIGHT) {
				if (state.id % 2 == 0 && state.snake1_direction != LEFT) { //if you're player 1
					state.snake1_direction = RIGHT;
				} else if (state.id % 2 == 1 && state.snake1_direction != LEFT) {
					state.snake2_direction = RIGHT;
				}
				return true;
		}
	}

	return false;
}

void GameMode::update(float elapsed) {
	if (state.gameOver && state.playerOne) {
		std::string message = "PLAYER ONE WINS";
		std::cout << message << std::endl;

		return;
	} else if (state.gameOver && !state.playerOne) {
		std::string message = "PLAYER TWO WINS";
		std::cout << message << std::endl;

		return;
	}

	 else {
		if (state.gameStarted && !state.gameOver) { //update only if game started
			state.update(elapsed);
		}

		if (client.connection) {
			//send game state to server
			ServerState temp_serverstate{};
			temp_serverstate.snake1 = state.snake1;
			temp_serverstate.snake2 = state.snake2;
			assert(temp_serverstate.snake1.size() > 0);
			assert(state.snake2.size() > 0);
			temp_serverstate.snake1_direction = state.snake1_direction;
			temp_serverstate.snake2_direction = state.snake2_direction;
			temp_serverstate.apples = state.apples;
			temp_serverstate.gameOver = state.gameOver;
			temp_serverstate.playerOne  = state.playerOne;
			SerializedState temp_serial{};
			state_to_serialized(&temp_serverstate, &temp_serial, state.id, true);
			client.connection.send_raw("s", 1);
			client.connection.send_raw(&temp_serial, sizeof(SerializedState));
		}

		//get state
		client.poll([&](Connection *c, Connection::Event event){
			if (event == Connection::OnOpen) {
				//probably won't get this.
			} else if (event == Connection::OnClose) {
				std::cerr << "Lost connection to server." << std::endl;
			} else { assert(event == Connection::OnRecv); //Recieve bytes from server
				if (c->recv_buffer[0] == 's') {
					if (c->recv_buffer.size() < 1 + sizeof(SerializedState)) {
						return; //wait for more data
					} else {
						SerializedState temp_ss{};
						ServerState temp_serverstate{};
						temp_serverstate.snake1.push_back(glm::vec2(1.0f, 1.0f));
						temp_serverstate.snake2.push_back(glm::vec2(19.0f, 15.0f));
						memcpy(&temp_ss, c->recv_buffer.data() + 1, sizeof(SerializedState));
						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 +sizeof(SerializedState));
						state.id = serialized_to_state(&temp_ss, &temp_serverstate, true, -5);
						//Save state you recieved
						state.apples = temp_serverstate.apples;
						state.gameOver = temp_serverstate.gameOver;
						state.playerOne = temp_serverstate.playerOne;
						state.gameStarted = temp_serverstate.gameStarted;
						state.snake1.clear();
						for(glm::vec2 elem : temp_serverstate.snake1) {
							state.snake1.push_back(elem);
						}
						state.snake2.clear();
						for(glm::vec2 elem : temp_serverstate.snake2) {
							state.snake2.push_back(elem);
						}
						assert(temp_serverstate.snake1.size() > 0);
						assert(state.snake2.size() > 0);
						//std::cout << "cli2" << std::endl;
						if (state.id % 2 == 0) { //if you're player one
							state.snake2_direction = temp_serverstate.snake2_direction;
						} else {
							state.snake1_direction = temp_serverstate.snake1_direction;
						}

					}

				}
			}
	});
}

//TODO render new balls somehow && case for snake
	//copy game state to scene positions:
	assert(state.snake1.size() > 0);
	assert(state.snake2.size() > 0);

	uint32_t i = 0;

	for (glm::vec2 apple_loc : state.apples) {
		apple_transforms[i]->position.x = (apple_loc.x/2.0f) - 5.0f;
		apple_transforms[i]->position.y = (apple_loc.y/2.0f) - 4.0f;
		i++;
	}
	i = 0;
	for (glm::vec2 ball_loc : state.snake1) {
		snake1_transforms[i]->position.x = (ball_loc.x/2.0f) - 5.0f;
		snake1_transforms[i]->position.y = (ball_loc.y/2.0f) - 4.0f;
		i++;
	}
	i = 0;
	for (glm::vec2 ball_loc : state.snake2) {
		snake2_transforms[i]->position.x = (ball_loc.x/2.0f) - 5.0f;
		snake2_transforms[i]->position.y = (ball_loc.y/2.0f) - 4.0f;
		i++;
	}

	assert(i <= 5);


}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	camera->aspect = drawable_size.x / float(drawable_size.y);

	glClearColor(0.25f, 0.0f, 0.5f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//set up light positions:
	glUseProgram(vertex_color_program->program);
	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));

	scene->draw(camera);


	GL_ERRORS();
}
