#pragma once 

#include "engine/common.hpp"

class APP {
public:
  	ENGINE::TEMPLATES::UniquePtr<ENGINE::COMMON::Scene> curscene;
};

extern APP g_app;