#include "ftxui/screen_interactive.hpp"
#include "ftxui/component/component.hpp"
#include "ftxui/component/delegate.hpp"
#include <iostream>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

namespace ftxui {

namespace {
  constexpr int ESC = 27;
  constexpr int WAT = 195;
  constexpr int WATWAIT = 91;

  Event GetEvent() {
    int v1 = getchar();
    if (v1 == ESC) {
      int v2 = getchar();
      int v3 = getchar();

      //if (v2 == WATWAIT) {
        //int v4 = getchar();
        //int v5 = getchar();
        //return Event{v1, v2, v3, v4, v5};
      //}
      return Event{v1, v2, v3};
    }

    if (v1 == WAT) {
      int v2 = getchar();
      return Event{v1, v2};
    }

    return Event{v1};
  };
};

class ScreenInteractive::Delegate : public component::Delegate {
 public:
  Delegate() : root_(this) {}

  void Register(component::Component* c) override { component_ = c; }

  std::vector<std::unique_ptr<Delegate>> child_;
  Delegate* NewChild() override {
    Delegate* child = new Delegate;
    child->root_ = root_;
    child->parent_ = this;

    if (!child_.empty()) {
      child_.back()->next_sibling_ = child;
      child->previous_sibling_ = child_.back().get();
    }

    child_.emplace_back(child);
    return child;
  }

  void OnEvent(Event event) { component_->OnEvent(event); }


  std::vector<component::Delegate*> children() override {
    std::vector<component::Delegate*> ret;
    for (auto& it : child_)
      ret.push_back(it.get());
    return ret;
  }

  Delegate* root_;
  Delegate* parent_ = nullptr;
  Delegate* previous_sibling_ = nullptr;
  Delegate* next_sibling_ = nullptr;
  component::Component* component_;

  Delegate* Root() override { return root_; }
  Delegate* Parent() override { return parent_; }
  Delegate* PreviousSibling() override { return previous_sibling_; }
  Delegate* NextSibling() override { return next_sibling_; }
  component::Component* component() override { return component_; }
};

ScreenInteractive::ScreenInteractive(size_t dimx, size_t dimy)
    : Screen(dimx, dimy), delegate_(new Delegate) {}
ScreenInteractive::~ScreenInteractive() {}

void ScreenInteractive::Loop() {
  std::cout << "\033[?9h";    /* Send Mouse Row & Column on Button Press */
  std::cout << "\033[?1000h"; /* Send Mouse X & Y on button press and release */
  std::cout << std::flush;

  // Save the old terminal configuration.
  struct termios terminal_configuration_old;
  tcgetattr(STDIN_FILENO, &terminal_configuration_old);

  // Set the new terminal configuration
  struct termios terminal_configuration_new;
  terminal_configuration_new = terminal_configuration_old;

  // Non canonique terminal.
  terminal_configuration_new.c_lflag &= ~ICANON;
  // Do not print after a key press.
  terminal_configuration_new.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &terminal_configuration_new);

  Draw();
  while (!quit_) {
    delegate_->OnEvent(GetEvent());

    Clear();
    Draw();
  }
  std::cout << std::endl;
  //Clear();

  // Restore the old terminal configuration.
  tcsetattr(STDIN_FILENO, TCSANOW, &terminal_configuration_old);
}

void ScreenInteractive::Draw() {
  auto document = delegate_->component()->Render();
  Render(*this, document.get());
  std::cout << ToString() << std::flush;
}

void ScreenInteractive::Clear() {
  std::cout << ResetPosition();
  Screen::Clear();
}

component::Delegate* ScreenInteractive::delegate() {
  return delegate_.get();
}

std::function<void()> ScreenInteractive::ExitLoopClosure() {
  return [this]() { quit_ = true; };
}

}  // namespace ftxui
