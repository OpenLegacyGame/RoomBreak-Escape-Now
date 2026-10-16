#pragma once
#include "axmol.h"
#include <functional>
class LoadingScreen final : public ax::Node
{
public:
    static LoadingScreen* create(int language, bool tips, std::function<void()> complete);

private:
    void setup(int language, bool tips, std::function<void()> complete);
};
