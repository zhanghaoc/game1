#include "PPU466.hpp"
#include "Mode.hpp"

#include <cstdint>
#include <glm/glm.hpp>

#include <stdint.h>
#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	enum bgColor: uint32_t {
		red = 0,
		green = 1,
		red_up = 2,
		red_down = 3,
		red_right = 4,
		red_left = 5,
		green_up = 6,
		green_down = 7,
		green_right = 8,
		green_left = 9,
		red_green = 10,
		
	};
	enum colorpalette: uint8_t {
		ogreen_tri_palette = 0,
		red_tri_palette = 1,
		orange_tri_palette = 2,
		blue_tri_palette = 3,
		yellow_tri_palette = 4,
		background_palette = 5,
		portal_img_palette = 6,
		player_palette = 7,
	};
	enum shape: uint8_t {
		flag = 255,
		trigger_heart = 254,
		player_shape = 238
	};
	enum bgPortal: uint8_t {
		p_r_left = 248,
		p_r_right = 247,
		p_r_down = 246,
		p_r_up = 245,
		p_g_left = 244,
		p_g_right = 243,
		p_g_down = 242,
		p_g_up = 241,
		red_area = 240,
		green_area = 239,
	};
	enum blockColor: uint32_t {
		green_block = 0,
		red_block = 1,
		orange_block = 2,
		blue_block = 3,
		yellow_block = 4,
	};
	enum direction: int {
		dir_left = 0,
		dir_right = 1,
		dir_up = 2,
		dir_down = 3,
		dir_none = 4
	};
	enum block_type: int {
		none_block = 249,
		left_block = 251,
		right_block = 250,
		up_block = 252,
		down_block = 253
	};

	struct door {
		bool up = false;
		bool down = false;
		bool left = false;
		bool right = false;
	};
	

	struct block {
		uint32_t x;
		uint32_t y;
		uint8_t block_type; // 0 for none, 1 for left, 2 for right, 3 for up, 4 for down
		uint8_t color;
	};
	struct movingBlockGroup {
		direction dir;
		std::vector<block> blocks;
		std::vector<int> ids;
	};
	struct trigger {
		uint32_t x;
		uint32_t y;
		uint8_t color;
		uint32_t id;
		direction dir; // 0 for left, 1 for right, 2 for up, 3 for down
		std::vector<block> blocks;
	};

	//some weird background animation:
	float background_fade = 0.0f;

	const unsigned int WIDTH = 32;
	const unsigned int HEIGHT = 30;
	float cooldown_time = 0.1f;
	float last_input_time = 0;
	unsigned int sprites_index = 0;
	// bool atRed = true;

	// red for 0, green for 1
	std::vector<std::vector<int>> map;
	// for block, use id, for trigger use -id
	std::vector<std::vector<int>> triggerMap;
	std::vector<std::vector<door>> doorMap;
	std::vector<trigger> triggers;

	uint16_t bgColorOffset = background_palette << 8;
	uint16_t colorIndex[10] = {red_area + (5 << 8), green_area + (5 << 8),
	 p_r_up + (5 << 8), p_r_down + (5 << 8), p_r_right + (5 << 8), p_r_left + (5 << 8),
	 p_g_up + (5 << 8), p_g_down + (5 << 8), p_g_right + (5 << 8), p_g_left + (5 << 8)};

	//player position:
	glm::uvec2 player_at = glm::uvec2(3, 3);

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
