
#include <iostream>

#include "Application.h"

Application::Application(/* args */)
{
    std::cout << "Application created" << std::endl;
}

Application::~Application()
{
    std::cout << "Application destroyed" << std::endl;
}

void Application::Run()
{
    std::cout << "Application running" << std::endl;
    std::cin.get();
}