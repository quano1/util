#include <libs/Signal.hpp>

#include <iostream>

class Person
{
public:
    Person(std::string const &name) : name_(name) {}
    Signal<std::string const &> say;

    void listen(std::string const &message)
    {
        std::cout << name_ << "received: " << message << std::endl;
    }

private:
    std::string name_;
};

class Button {
 public:
  Signal<> on_click;
};

class Message {
 public:
  void display() const {
    std::cout << "Hello World!" << std::endl;
  }
};

void test1()
{
    Signal<std::string, int> signal;
    signal.connect([](std::string arg1, int arg2)
    {
        std::cout << arg1 << " " << arg2 << std::endl;
    });

    signal.emit("The answer:", 42);
}

void test2()
{
  Button  button;
  Message message;

  button.on_click.connect_member(&message, &Message::display);
  button.on_click.emit();
}

void test3()
{
  Person alice("Alice"), bob("Bob");

  alice.say.connect_member(&bob, &Person::listen);
  bob.say.connect_member(&alice, &Person::listen);

  alice.say.emit("Have a nice day!");
  bob.say.emit("Thank you!");
}

void test4()
{
    Property<float> InputValue;
    Property<float> OutputValue;
    Property<bool> CriticalSituation;

    OutputValue.connect_from(InputValue);
    // InputValue.connect(OutputValue);

    OutputValue.on_change().connect([&](float val)
    {
        std::cout << "Output: " << val << std::endl;
        if (val > 0.5f)
        {
            CriticalSituation = true;
        }
        else
        {
            CriticalSituation = false;
        }
    });

    CriticalSituation.on_change(). connect([](bool val)
    {
        if (val)
        {
            std::cout << "Danger danger!" << std::endl;
        }
    });

    InputValue = 0.2;
    InputValue = 0.4;
    InputValue = 0.6;

}

int main()
{
    test1();
    test2();
    test3();
    test4();
    return 0;
}