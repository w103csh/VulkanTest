
#include "InputHandler.h"

#include <sstream>

#include "Helpers.h"

namespace {

const bool MY_DEBUG = false;

// move
constexpr float K_X_MOVE_FACT = 2.0f;
constexpr float K_Y_MOVE_FACT = 2.0f;
constexpr float K_Z_MOVE_FACT = 2.0f;
// look
constexpr float M_X_LOOK_FACT = 0.1f;
constexpr float M_Y_LOOK_FACT = -0.1f;

}  // namespace

Input::Handler::Handler(Shell* pShell)
    : Shell::Handler(pShell),
      currKeyInput_(),
      posDir_(),
      isLooking_(false),
      hasFocus_(false),
      currMouseInput_{0.0f, 0.0f, 0.0f},
      prevMouseInput_{0.0f, 0.0f, 0.0f},
      lookDir_() {}

void Input::Handler::updateInput(float elapsed) {
    // reset();

    updateKeyInput();
    updateMouseInput();

    // account for time
    posDir_ *= elapsed;
    // lookDir_ *= (0.001 / elapsed);

    if (MY_DEBUG && glm::any(glm::notEqual(posDir_, glm::vec3(0.0f)))) {
        std::stringstream ss;
        ss << "move (" << elapsed << "):";
        sh_->log(Shell::LOG_INFO, helpers::makeVec3String(ss.str(), posDir_).c_str());
    }
}

void Input::Handler::updateKeyInput() {
    std::stringstream ss;

    for (auto& key : currKeyInput_) {
        switch (key) {
                // FORWARD
            case GAME_KEY::KEY_UP:
            case GAME_KEY::KEY_W: {
                if (MY_DEBUG) ss << " FORWARD ";
                posDir_.z += K_Z_MOVE_FACT;
            } break;
                // BACK
            case GAME_KEY::KEY_DOWN:
            case GAME_KEY::KEY_S: {
                if (MY_DEBUG) ss << " BACK ";
                posDir_.z += K_Z_MOVE_FACT * -1;
            } break;
                // RIGHT
            case GAME_KEY::KEY_RIGHT:
            case GAME_KEY::KEY_D: {
                if (MY_DEBUG) ss << " RIGHT ";
                posDir_.x += K_X_MOVE_FACT;
            } break;
                // LEFT
            case GAME_KEY::KEY_LEFT:
            case GAME_KEY::KEY_A: {
                if (MY_DEBUG) ss << " LEFT ";
                posDir_.x += K_X_MOVE_FACT * -1;
            } break;
                // UP
            case GAME_KEY::KEY_E: {
                if (MY_DEBUG) ss << " UP ";
                posDir_.y += K_Y_MOVE_FACT;
            } break;
                // DOWN
            case GAME_KEY::KEY_Q: {
                if (MY_DEBUG) ss << " DOWN ";
                posDir_.y += K_Y_MOVE_FACT * -1;
            } break;
            default:;
        }
    }

    if (MY_DEBUG && ss.str().size() > 0) {
        ss << "\n Y position direction: ";
        sh_->log(Shell::LOG_INFO, helpers::makeVec3String(ss.str(), posDir_).c_str());
    }
}

void Input::Handler::updateMouseInput() {
    std::stringstream ss;

    if (isLooking_) {
        lookDir_.x = (currMouseInput_.xPos - prevMouseInput_.xPos) * M_X_LOOK_FACT;
        lookDir_.y = (currMouseInput_.yPos - prevMouseInput_.yPos) * M_Y_LOOK_FACT;

        if (MY_DEBUG) {
            ss << "look direction: ";
            ss << helpers::makeVec3String(ss.str(), posDir_);
        }

        if (MY_DEBUG && (!helpers::almost_equal(lookDir_.x, 0.0f, 1) || !helpers::almost_equal(lookDir_.y, 0.0f, 1))) {
            ss << "look direction: ";
            ss << helpers::makeVec3String(ss.str(), lookDir_);
        }
    }
    if (currMouseInput_.zDelta) {
        posDir_.z += currMouseInput_.zDelta > 0 ? K_Z_MOVE_FACT * 6 : K_Z_MOVE_FACT * -6;
        currMouseInput_.zDelta = 0;  // reset zDelta here... this stuff is not great

        if (MY_DEBUG) {
            ss << helpers::makeVec3String("mouse wheel: ", posDir_);
        }
    }

    if (MY_DEBUG && ss.str().size() > 0) {
        sh_->log(Shell::LOG_INFO, ss.str().c_str());
    }

    prevMouseInput_ = currMouseInput_;
    isLooking_ = false;
}

void Input::Handler::reset() {
    lookDir_ = {};
    posDir_ = {};
    currMouseInput_.moving = false;
}